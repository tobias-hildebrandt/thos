BUILD ?= build
SRC ?= src
MISC ?= misc

COMP_DB_FILENAME := compile_commands.json
COMP_DB_PART_DIR := compdb

COMP_DB := ${BUILD}/${COMP_DB_FILENAME}
COMP_DB_ALL_PARTS = $(shell find ${BUILD}/${COMP_DB_PART_DIR}/ -type f)

OPTIMIZE ?= -O2
DEBUG ?= -g3
WARNINGS ?= -Wall -Wextra -Wpedantic -Wformat=2 \
	-Wno-gnu-zero-variadic-macro-arguments -Wimplicit-fallthrough

CC := clang
LD := ld.ldd
OBJCOPY := llvm-objcopy
OBJDUMP := llvm-objdump

COMMON_CFLAGS := -std=c11 \
	--target=riscv64-unknown-elf -mcmodel=medany -march=rv64g \
	-fno-stack-protector -ffreestanding -nostdlib \
	${OPTIMIZE} ${DEBUG} \
	${WARNINGS} \
	-I ${SRC}/common/ \
	-isystem ${SRC}/libc/

KERNEL_CFLAGS := ${COMMON_CFLAGS} \
	-I ${SRC}/kernel/ \
	${KERNEL_CFLAGS_EXTRA}

USER_CFLAGS := ${COMMON_CFLAGS} -I ${SRC}/userlib/

# function that takes a list of C sources and returns list of object files
obj_fn = $(patsubst %.c,%.o,$(patsubst ${SRC}%, ${BUILD}%, $(1)))

# function that takes a path of a build object and returns its comp db part path
compdb_path_fn = ${BUILD}/${COMP_DB_PART_DIR}/$(shell echo $(1) | tr '/' '_').compdbpart

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

QEMU := qemu-system-riscv64
QEMU_FLAGS := -machine virt -bios default -nographic -serial mon:stdio --no-reboot -kernel ${KERNEL_ELF}

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

.PHONY: kernel
kernel: ${KERNEL_ELF} ${COMP_DB} ${KERNEL_OBJDUMP} ${USER_OBJDUMPS}

.PHONY: run
run: qemu

# run kernel via qemu
.PHONY: qemu
qemu: kernel
	${QEMU} ${QEMU_FLAGS}

# generate the gdb init file
${GDB_INIT_FILE}:
	@sed 's/:PORT_GOES_HERE/:${GDB_PORT}/' < ${GDB_INIT_TEMPLATE} > $@

# run kernel via qemu and wait for a remote gdb to attach
.PHONY: qemu-dbg
qemu-dbg: qemu-gdb
.PHONY: qemu-gdb
qemu-gdb: kernel ${GDB_INIT_FILE}
	@echo "-----"
	@echo "gdb port is: ${GDB_PORT}"
	@echo "launch gdb with: gdb-multiarch -ix ${GDB_INIT_FILE} ${KERNEL_ELF}"
	@echo "or launch CodeLLDB debug configuration"
	@echo
	@echo "(CTRL+A X to exit)"
	@echo "-----"
	@echo
	${QEMU} ${QEMU_FLAGS} -S -gdb tcp::${GDB_PORT}

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
		-Wl,-T${KERNEL_LINKER_SCRIPT} -Wl,-Map=${BUILD}/kernel.map -fuse-ld=lld \
		-o $@ ${KERNEL_OBJS} ${LIBC_OBJS} ${USER_BLOBS}

# kernel object
${BUILD}/kernel/%.o: ${SRC}/kernel/%.c ${KERNEL_HEADERS} ${LIBC_HEADERS} | ${BUILD}
	${CC} ${KERNEL_CFLAGS} -c $< -o $@ -MJ $(call compdb_path_fn, $@)

# libc object
${BUILD}/libc/%.o: ${SRC}/libc/%.c ${LIBC_HEADERS} | ${BUILD}
	${CC} ${KERNEL_CFLAGS} -c $< -o $@ -MJ $(call compdb_path_fn, $@)

## user programs

# can't use objcopy for bin->blob since ABI gets lost in translation
# https://github.com/llvm/llvm-project/issues/68915

# userlib
${BUILD}/userlib/%.o: ${SRC}/userlib/%.c ${USERLIB_HEADERS} | ${BUILD}
	${CC} ${USER_CFLAGS} -c $< -o $@ -MJ $(call compdb_path_fn, $@)

# step 1
# user object
${BUILD}/user/%.o: ${SRC}/user/%.c ${USERLIB_HEADERS} | ${BUILD}
	${CC} ${USER_CFLAGS} -c $< -o $@ -MJ $(call compdb_path_fn, $@)

# step 2
# user ELF binary
${BUILD}/user/%.elf: ${BUILD}/user/%.o ${LIBC_OBJS} ${USERLIB_OBJS} | ${BUILD}
	${CC} ${USER_CFLAGS} \
		-Wl,-T${USER_LINKER_SCRIPT} -Wl,-Map=${BUILD}/user/$(shell basename $@ .elf).map -fuse-ld=lld \
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
