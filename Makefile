BUILD ?= build
SRC ?= src
MISC ?= misc

COMP_DB := ${BUILD}/compile_commands.json
COMP_DB_PART := ${COMP_DB}.part

OPTIMIZE ?= -O2
DEBUG ?= -g3
WARNINGS ?= -Wall -Wextra -Wpedantic \
	-Wno-gnu-zero-variadic-macro-arguments

CC := clang
CFLAGS := -std=c11 -fuse-ld=lld \
	--target=riscv64-unknown-elf -mcmodel=medany -march=rv64g \
	-fno-stack-protector -ffreestanding -nostdlib \
	-MJ ${COMP_DB_PART} \
	-I${SRC}/kernel/ \
	-isystem${SRC}/libc/ \
	${OPTIMIZE} ${DEBUG} \
	${WARNINGS} \
	${CFLAGS_EXTRA}

KERNEL_LINKER_SCRIPT := ${SRC}/kernel/kernel.lds
KERNEL_C_SOURCES := $(shell find ${SRC}/kernel/ -name '*.c')
KERNEL_HEADERS := $(shell find ${SRC}/kernel/ -name '*.h')
KERNEL_ELF := ${BUILD}/kernel.elf

LIBC_C_SOURCES := $(shell find ${SRC}/libc/ -name '*.c')
LIBC_HEADERS := $(shell find ${SRC}/libc/ -name '*.h')

QEMU := qemu-system-riscv64
QEMU_FLAGS := -machine virt -bios default -nographic -serial mon:stdio --no-reboot -kernel ${KERNEL_ELF}

GDB_INIT_TEMPLATE := ${MISC}/gdbinit-template
GDB_PORT ?= 7777

GDB_INIT_FILE := ${BUILD}/gdbinit

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

.PHONY: kernel
kernel: ${KERNEL_ELF} ${COMP_DB}

.PHONY: run
run: qemu

# run kernel via qemu
.PHONY: qemu
qemu: kernel
	${QEMU} ${QEMU_FLAGS}

# generate the gdb init file
.PHONY: ${GDB_INIT_FILE}
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

# kernel ELF binary
${KERNEL_ELF}: ${KERNEL_C_SOURCES} ${KERNEL_HEADERS} ${KERNEL_LINKER_SCRIPT} ${BUILD} ${LIBC_C_SOURCES} ${LIBC_HEADERS}
	${CC} ${CFLAGS} -Wl,-T${KERNEL_LINKER_SCRIPT} -Wl,-Map=${BUILD}/kernel.map -o $@ ${KERNEL_C_SOURCES} ${LIBC_C_SOURCES}

# compilation commands database for clangd
${COMP_DB}: ${KERNEL_ELF}
	@printf "[\n" > ${COMP_DB}
	@cat ${COMP_DB_PART} >> ${COMP_DB}
	@truncate -s-2 ${COMP_DB}
	@printf "\n]" >> ${COMP_DB}

# create the build dir itself
${BUILD}:
	mkdir -p ${BUILD}
