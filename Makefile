# This Makefile is essentially just a command runner.
# meson does the actual build (see meson.build).

### variables

# default to -j$(nproc)
ifeq (,$(findstring j, $(MAKEFLAGS)))
	MAKEFLAGS += -j$(shell nproc)
# also set JOBS (passed to meson compile)
	JOBS := $(shell nproc)
endif

# build architecture & toolchain
TARGET ?= riscv64
TOOLCHAIN ?= llvm

# build dir
BUILD_BASE ?= build
BUILD := ${BUILD_BASE}/${TARGET}-${TOOLCHAIN}

# the resulting kernel file made by meson
KERNEL_ELF := ${BUILD}/kernel

# meson args
COMPILE_ARGS ?=
SETUP_ARGS ?=
DEFINES ?=
ALL_SETUP_ARGS := ${SETUP_ARGS} -Ddefines='${DEFINES}'
CROSS_FILE := misc/meson-machines/${TARGET}-${TOOLCHAIN}.txt
COMPILE_WRAPPER ?= misc/rewrite_paths.sh

# opensbi stuff
opensbi_fw_fn = ${BUILD_BASE}/opensbi/opensbi-$(1)-generic-fw_dynamic.bin
OPENSBI_FIRMWARES := $(call opensbi_fw_fn,riscv64) $(call opensbi_fw_fn,riscv32)
OPENSBI_DIR := subprojects/opensbi
OPENSBI_FW_OPTIONS ?= 0x1

# qemu stuff
QEMU ?= qemu-system-${TARGET}
QEMU_WRAP ?= misc/wrap_qemu.sh
QEMU_FW := $(call opensbi_fw_fn,${TARGET})
QEMU_BOOTARGS ?=
QEMU_FLAGS ?= -machine virt -bios ${QEMU_FW} \
	-nographic -serial mon:stdio \
	-kernel ${KERNEL_ELF} ${QEMU_BOOTARGS}
QEMU_OUTFILE := ${BUILD_BASE}/out

# clangd IDE support
COMP_DB_FILENAME := compile_commands.json

# gdb debugger support
GDB_INIT_TEMPLATE := misc/gdbinit-template
GDB_INIT_FILE := ${BUILD_BASE}/gdbinit
GDB_PORT ?= 7777

### general

# `make` runs build
.PHONY: default
default: build

# prints targets and arguments
.PHONY: help
help:
	@echo "make targets and aliases:"
	@echo "setup (configure)    configure build directory"
	@echo "build (compile)      build the kernel"
	@echo "opensbi (firmware)   build firmware"
	@echo "qemu (run)           run kernel on qemu"
	@echo "qemu-gdb (qemu-dbg)  same as qemu, but wait for gdb to attach"
	@echo
	@echo "make arguments:"
	@echo "TARGET               target platform, riscv64 or riscv32"
	@echo "TOOLCHAIN            c toolchain to use, llvm or gnu"
	@echo "DEFINES              c defines, see src/kernel/flags.h"
	@echo "SETUP_ARGS           meson setup args, e.g. --optimization=3"
	@echo "COMPILE_ARGS         meson compile args, e.g. --verbose"
	@echo "QEMU_BOOTARGS:       qemu args, e.g. -append somebootargs"

# just makes a symlink to last-setup-target's compile_commands.json
.PHONY: link-compdb
link-compdb:
	mkdir -p ${BUILD_BASE}
	ln -sf ${PWD}/${BUILD}/${COMP_DB_FILENAME} ${PWD}/${BUILD_BASE}/${COMP_DB_FILENAME}

### meson

# meson setup, essentially configure
.PHONY: setup configure
configure: setup
setup: link-compdb
	meson setup ${BUILD} \
		--cross-file ${CROSS_FILE} \
		--reconfigure \
		${ALL_SETUP_ARGS}

# meson compile
.PHONY: build compile
compile: build
build: setup
	${COMPILE_WRAPPER} meson compile -C ${BUILD} ${COMPILE_ARGS} -j ${JOBS}

### clean

# clean current target+toolchain build dir
.PHONY: clean
clean:
	rm -rf ${BUILD}

# clean all targets+toolchains build dirs
.PHONY: clean-all
clean-all:
	rm -rf ${BUILD_BASE}

### QEMU

# run kernel via qemu
# only builds needed firmware
.PHONY: qemu run
run: qemu
qemu: build ${QEMU_FW}
	${QEMU_WRAP} ${QEMU_OUTFILE} ${QEMU} ${QEMU_FLAGS}

# generate the gdb init file
${GDB_INIT_FILE}:
	mkdir -p ${BUILD_BASE}
	sed 's/:PORT_GOES_HERE/:${GDB_PORT}/' < ${GDB_INIT_TEMPLATE} > $@

# run kernel via qemu and wait for a remote gdb to attach
.PHONY: qemu-gdb qemu-dbg
qemu-dbg: qemu-gdb
qemu-gdb: build ${QEMU_FW} ${GDB_INIT_FILE}
	@echo "gdb port is: ${GDB_PORT}"
	@echo "launch gdb with:"
	@echo
	@echo "gdb-multiarch -ix ${GDB_INIT_FILE} ${KERNEL_ELF}"
	@echo
	@echo "(CTRL+A X to exit)"
	${QEMU_WRAP} ${QEMU_OUTFILE} ${QEMU} ${QEMU_FLAGS} -S -gdb tcp::${GDB_PORT}

### OpenSBI

# builds both firmwares
.PHONY: opensbi firmware
firmware: opensbi
opensbi: ${OPENSBI_FIRMWARES}

# cleans firmwares and opensbi build artifacts
.PHONY: clean-opensbi
clean-opensbi:
	rm -rf ${OPENSBI_DIR}/build/
	rm -rf ${OPENSBI_FIRMWARES}

# initializes opensbi submodule
${OPENSBI_DIR}:
	git submodule update --init --recursive

# builds and links single opensbi firmware image
${OPENSBI_FIRMWARES}: ${BUILD_BASE}/opensbi/opensbi-riscv%-generic-fw_dynamic.bin: ${OPENSBI_DIR}
	mkdir -p ${OPENSBI_DIR}/build
	mkdir -p ${BUILD_BASE}/opensbi
	$(MAKE) -C ${OPENSBI_DIR} \
		CROSS_COMPILE=riscv$*-unknown- PLATFORM_RISCV_XLEN=$* \
		PLATFORM=generic FW_OPTIONS=${OPENSBI_FW_OPTIONS} LLVM=1 O=build/$*/
	ln -sf ${PWD}/${OPENSBI_DIR}/build/$*/platform/generic/firmware/fw_dynamic.bin \
		${PWD}/${QEMU_FW}
