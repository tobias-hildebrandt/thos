BUILD ?= build
SRC ?= src
MISC ?= misc

COMP_DB_FILENAME := compile_commands.json
COMP_DB_PART_DIR := compdb

COMP_DB := ${BUILD}/${COMP_DB_FILENAME}
COMP_DB_ALL_PARTS = $(shell find ${BUILD}/${COMP_DB_PART_DIR}/ -type f)
compdbpart_fn = ${BUILD}/${COMP_DB_PART_DIR}/$(shell echo $(1) | tr '/' '_').compdbpart

PLATFORM ?= qemu64
LIBGCC_PREFIX ?= /usr/lib/gcc/riscv64-unknown-elf/14.2.0

ifeq (${PLATFORM}, qemu64)
	CLANG_TARGET := riscv64-unknown-elf
	GCC := riscv64-unknown-elf-gcc
	PLATFORM_CFLAGS := -mcmodel=medany -march=rv64g -mabi=lp64d -DQEMU64=1
	QEMU := qemu-system-riscv64
else ifeq (${PLATFORM}, qemu32)
	CLANG_TARGET := riscv32-unknown-elf
	GCC := riscv64-unknown-elf-gcc
	PLATFORM_CFLAGS := -mcmodel=medany -march=rv32g -mabi=ilp32d -DQEMU32=1
	LINK_ARGS := -Wl,-L${LIBGCC_PREFIX}/rv32iafd/ilp32d -lgcc
	QEMU := qemu-system-riscv32
else
# tab indentation not allowed
$(error Unsupported platform: ${PLATFORM})
endif

OPTIMIZE ?= -O2
DEBUG ?= -g3
WARNINGS ?= -Wall -Wextra -Wpedantic -Wformat=2 \
	-Wimplicit-fallthrough

ifndef USE_GCC
	CC := clang
	COMMON_CFLAGS := --target=${CLANG_TARGET}
	compdb_cflag_fn = -MJ $(call compdbpart_fn, $(1))
	compdb_cc_wrap_fn =
	WARNINGS += -Wno-gnu-zero-variadic-macro-arguments
else
	CC := ${GCC}
	COMMON_CFLAGS :=
	compdb_cflag_fn =
	compdb_cc_wrap_fn = bear --output $(call compdbpart_fn, $(1)) --
endif

# gnu binutil's objcopy and objdump don't work!
OBJCOPY := llvm-objcopy
OBJDUMP := llvm-objdump

COMMON_CFLAGS += -std=gnu17 \
	${PLATFORM_CFLAGS} \
	-fno-stack-protector -ffreestanding -nostdlib \
	${OPTIMIZE} ${DEBUG} \
	${WARNINGS} \
	-I ${SRC}/common/ \
	-isystem ${SRC}/libc/

KERNEL_CFLAGS := ${COMMON_CFLAGS} \
	-I ${SRC}/kernel/ \
	${KERNEL_CFLAGS_EXTRA}

USER_CFLAGS := ${COMMON_CFLAGS} -I ${SRC}/userlib/ -Wno-empty-translation-unit

# function that takes a list of C sources and returns list of object files
obj_fn = $(patsubst %.c,%.o,$(patsubst ${SRC}%, ${BUILD}%, $(1)))

KERNEL_LINKER_SCRIPT := ${SRC}/kernel/kernel.lds
KERNEL_C_SOURCES := $(shell find ${SRC}/kernel/ -name '*.c')
KERNEL_HEADERS := $(shell find ${SRC}/kernel/ -name '*.h')
KERNEL_OBJS := $(call obj_fn, ${KERNEL_C_SOURCES})
KERNEL_ELF := ${BUILD}/kernel.elf
KERNEL_OBJDUMP := ${BUILD}/kernel.objdump

LIBC_C_SOURCES := $(shell find ${SRC}/libc/ -name '*.c')
LIBC_HEADERS := $(shell find ${SRC}/libc/ -name '*.h')
LIBC_OBJS := $(call obj_fn, ${LIBC_C_SOURCES})

USERLIB_C_SOURCES := $(shell find ${SRC}/userlib/ -name '*.c')
USERLIB_HEADERS := $(shell find ${SRC}/userlib/ -name '*.h')
USERLIB_OBJS := $(call obj_fn, ${USERLIB_C_SOURCES})

USER_LINKER_SCRIPT := ${SRC}/user/user.lds
USER_C_SOURCES := $(shell find ${SRC}/user/ -name '*.c')
USER_OBJS := $(call obj_fn, ${USER_C_SOURCES})
USER_BLOBS := $(USER_OBJS:.o=.blob)
USER_OBJDUMPS := $(USER_OBJS:.o=.objdump)
# TODO: pass into C somehow? and do same with sections through nm/objdump?
#USER_PROGS := $(shell basename -a $(USER_OBJS:.o=))

QEMU_FLAGS := -machine virt -bios default -nographic -serial mon:stdio --no-reboot -kernel ${KERNEL_ELF}

OUTFILE := ${BUILD}/out

GDB_INIT_TEMPLATE := ${MISC}/gdbinit-template
GDB_PORT ?= 7777

GDB_INIT_FILE := ${BUILD}/gdbinit

# default to -j$(nproc)
ifeq (,$(findstring j, $(MAKEFLAGS)))
	MAKEFLAGS += -j$(shell nproc)
endif

# keep all intermediate targets
.SECONDARY:

.PHONY: default
default: kernel

.PHONY: help
help:
	@echo "make targets:"
	@echo
	@echo "kernel (default)     build the kernel ELF"
	@echo "run (or qemu)        run the kernel via QEMU"
	@echo "qemu-gdb             run the kernel via QEMU with GDB remote debugging"
	@echo "clean                clean the build directory"
	@echo "vars                 print Makefile vars"

.PHONY: vars
vars:
	@echo "MAKEFLAGS:       ${MAKEFLAGS}"
	@echo "KERNEL_ELF:      ${KERNEL_ELF}"
	@echo "KERNEL_OBJS:     ${KERNEL_OBJS}"
	@echo "LIBC_OBJS:       ${LIBC_OBJS}"
	@echo "USER_OBJS:       ${USER_OBJS}"
	@echo "USER_BLOBS:      ${USER_BLOBS}"
	@echo "USERLIB_OBJS:    ${USERLIB_OBJS}"

.PHONY: cdefines
cdefines:
	@echo | ${CC} ${KERNEL_CFLAGS} -dM -E - | sed -r 's/^#define //'

.PHONY: kernel
kernel: ${KERNEL_ELF} ${COMP_DB} ${KERNEL_OBJDUMP} ${USER_OBJDUMPS}

.PHONY: run
run: qemu

# run kernel via qemu
.PHONY: qemu
qemu: kernel ${OUTFILE}
	${QEMU} ${QEMU_FLAGS} | tee ${OUTFILE}

# generate the gdb init file
${GDB_INIT_FILE}:
	@sed 's/:PORT_GOES_HERE/:${GDB_PORT}/' < ${GDB_INIT_TEMPLATE} > $@

# run kernel via qemu and wait for a remote gdb to attach
.PHONY: qemu-dbg
qemu-dbg: qemu-gdb
.PHONY: qemu-gdb
qemu-gdb: kernel ${GDB_INIT_FILE} ${OUTFILE}
	@echo "-----"
	@echo "gdb port is: ${GDB_PORT}"
	@echo "launch gdb with: gdb-multiarch -ix ${GDB_INIT_FILE} ${KERNEL_ELF}"
	@echo "or launch CodeLLDB debug configuration"
	@echo
	@echo "(CTRL+A X to exit)"
	@echo "-----"
	@echo
	${QEMU} ${QEMU_FLAGS} -S -gdb tcp::${GDB_PORT} | tee ${OUTFILE}

${OUTFILE}: | ${BUILD}
	touch $@

# clean build dir
.PHONY: clean
clean:
	rm -rf ${BUILD}

# create the build dir structure
# should only be used as order-only prerequisite
${BUILD}:
	@mkdir -p ${BUILD}
	@mkdir -p ${BUILD}/kernel
	@mkdir -p ${BUILD}/libc
	@mkdir -p ${BUILD}/user
	@mkdir -p ${BUILD}/userlib
	@mkdir -p ${BUILD}/${COMP_DB_PART_DIR}

# compilation commands database for clangd
${COMP_DB}: ${KERNEL_ELF}  | ${BUILD}
ifdef USE_GCC
# horrible bear kludge
	@for file in ${COMP_DB_ALL_PARTS}; do \
		sed -i '1d;$$d' $$file; \
		echo "," >> $$file; \
	done
endif
	@printf "[\n" > ${COMP_DB}
	@cat ${COMP_DB_ALL_PARTS} >> ${COMP_DB}
	@truncate -s-2 ${COMP_DB}
	@printf "\n]" >> ${COMP_DB}

# kernel disassembly
${KERNEL_OBJDUMP}: ${KERNEL_ELF} | ${BUILD}
	${OBJDUMP} -D $< > $@

# kernel ELF binary
${KERNEL_ELF}: ${KERNEL_OBJS} ${LIBC_OBJS} ${USER_BLOBS} ${KERNEL_LINKER_SCRIPT} | ${BUILD}
	${CC} ${KERNEL_CFLAGS} \
		-fuse-ld=lld \
		-Wl,-T${KERNEL_LINKER_SCRIPT} -Wl,-Map=${BUILD}/kernel.map ${LINK_ARGS} \
		-o $@ ${KERNEL_OBJS} ${LIBC_OBJS} ${USER_BLOBS}

# kernel object
${BUILD}/kernel/%.o: ${SRC}/kernel/%.c ${KERNEL_HEADERS} ${LIBC_HEADERS} | ${BUILD}
	$(call compdb_cc_wrap_fn, $@) ${CC} ${KERNEL_CFLAGS} -c $< -o $@ $(call compdb_cflag_fn, $@)

# libc object
${BUILD}/libc/%.o: ${SRC}/libc/%.c ${LIBC_HEADERS} | ${BUILD}
	$(call compdb_cc_wrap_fn, $@) ${CC} ${KERNEL_CFLAGS} -c $< -o $@ $(call compdb_cflag_fn, $@)

## user programs

# can't use objcopy for bin->blob since ABI gets lost in translation
# https://github.com/llvm/llvm-project/issues/68915

# userlib
${BUILD}/userlib/%.o: ${SRC}/userlib/%.c ${USERLIB_HEADERS} | ${BUILD}
	$(call compdb_cc_wrap_fn, $@) ${CC} ${USER_CFLAGS} -c $< -o $@ $(call compdb_cflag_fn, $@)

# step 1
# user object
${BUILD}/user/%.o: ${SRC}/user/%.c ${USERLIB_HEADERS} | ${BUILD}
	$(call compdb_cc_wrap_fn, $@) ${CC} ${USER_CFLAGS} -c $< -o $@ $(call compdb_cflag_fn, $@)

# step 2
# user ELF binary
${BUILD}/user/%.elf: ${BUILD}/user/%.o ${LIBC_OBJS} ${USERLIB_OBJS} | ${BUILD}
	${CC} ${USER_CFLAGS} \
		-fuse-ld=lld \
		-Wl,-T${USER_LINKER_SCRIPT} -Wl,-Map=${BUILD}/user/$(shell basename $@ .elf).map ${LINK_ARGS} \
		-o $@ ${LIBC_OBJS} ${USERLIB_OBJS} $<

# step 3
# user BIN (fully-expanded memory image)
${BUILD}/user/%.bin: ${BUILD}/user/%.elf | ${BUILD}
	${OBJCOPY} --set-section-flags *=alloc,contents --output-target=binary $< $@

# step 4
# user BIN dummy assembler file
${BUILD}/user/%.S: ${BUILD}/user/%.bin | ${BUILD}
	@printf "" > $@
	@echo "# autogenerated assembler file for user program $(shell basename $@ .S)" >> $@
	@echo ".global __USER_$(shell basename $@ .S)_START" >> $@
	@echo ".global __USER_$(shell basename $@ .S)_END" >> $@
	@echo ".p2align 12 # 2^12 = 4096" >> $@
	@echo "__USER_$(shell basename $@ .S)_START:" >> $@
	@echo ".incbin \"$<\"" >> $@
	@echo "__USER_$(shell basename $@ .S)_END:" >> $@

# step 5
# user BIN binary as linkable blob
${BUILD}/user/%.blob: ${BUILD}/user/%.S ${BUILD}/user/%.bin | ${BUILD}
	${CC} ${USER_CFLAGS} -c $< -o $@

# user disassembly
${BUILD}/user/%.objdump: ${BUILD}/user/%.elf | ${BUILD}
	${OBJDUMP} -D $< > $@
