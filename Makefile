# This Makefile is essentially just a command runner.
# meson does the actual build (see meson.build).

### variables

# default to -j$(nproc)
ifeq (,$(findstring j, $(MAKEFLAGS)))
	MAKEFLAGS += -j$(shell nproc)
# also set JOBS (passed to meson compile)
	JOBS := -j$(shell nproc)
endif

# build architecture, toolchain, and board
TARGET ?= riscv64
TOOLCHAIN ?= llvm
MACHINE ?= virt

# build dir
BUILD_BASE ?= build
BUILD := ${BUILD_BASE}/${TARGET}-${TOOLCHAIN}-${MACHINE}

# kernel file made by meson
KERNEL_ELF := ${BUILD}/kernel
# kernel test file made by meson
KERNEL_ELF_TEST := ${BUILD}/kernel-test

# opensbi stuff
OPENSBI_DIR := subprojects/opensbi
OPENSBI_FW_OPTIONS ?= 0x1
OPENSBI_FW_TYPE ?= dynamic

# qemu stuff
QEMU := ${BUILD}/qemu-launch-${OPENSBI_FW_TYPE}.sh # built by meson
QEMU_WRAP ?= misc/wrap_qemu.sh
QEMU_OUTFILE := ${BUILD_BASE}/out
QEMU_FLAGS :=

# meson args
SETUP_ARGS ?=
COMPILE_ARGS ?=
TEST_ARGS ?= # args passed to `meson test`, not `meson setup -D test-args=`
DEFINES ?=
TEST_PREFIX_ARGS ?=

# force specific variables based on machine
ifeq ($(strip ${MACHINE}),sifive_u)
override DEFINES += USE_SBI_EXIT=1 USE_SBI_SET_TIMER=1
override QEMU_PARTIAL_FLAGS += -no-reboot
override TEST_PREFIX_ARGS := --parse-exitcode
endif

ALL_SETUP_ARGS ?= ${SETUP_ARGS} \
	-D defines='${DEFINES}' \
	-D enable-tests=true \
	-D machine='${MACHINE}' \
	-D opensbi:fw-type='${OPENSBI_FW_TYPE}' \
	-D opensbi:fw-options='${OPENSBI_FW_OPTIONS}' \
	-D opensbi:verbose="$$(\
	echo '${COMPILE_ARGS}' | grep -q -e '--verbose' && echo true || echo false\
	)"

CROSS_FILE_DIR := misc/meson-machines
CROSS_FILE_COMMON := ${CROSS_FILE_DIR}/riscv_common.ini
CROSS_FILE_TARGET := ${CROSS_FILE_DIR}/${TARGET}.ini
CROSS_FILE_TOOLCHAIN := ${CROSS_FILE_DIR}/${TOOLCHAIN}.ini
CROSS_FILE_FINAL := ${CROSS_FILE_DIR}/final.ini

COMPILE_WRAPPER ?= misc/rewrite_paths.sh

# clangd IDE support
COMP_DB_FILENAME := compile_commands.json

# gdb debugger support
GDB_INIT_TEMPLATE := misc/gdbinit-template
GDB_INIT_FILE := ${BUILD_BASE}/gdbinit
GDB_PORT ?= 7777

# device tree parser
DEVICETREE_SCRIPT := misc/parse-device-tree.sh

# objdump
OBJDUMP ?= llvm-objdump
DUMP ?= 1

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
	@echo "qemu (run)           run kernel on qemu"
	@echo "qemu-gdb (qemu-dbg)  same as qemu, but wait for gdb to attach"
	@echo "test                 build test kernel, run tests on qemu"
	@echo
	@echo "make arguments:"
	@echo "TARGET               target platform, riscv64 or riscv32"
	@echo "TOOLCHAIN            c toolchain to use, llvm or gnu"
	@echo "MACHINE              qemu machine target, virt or sifive_u"
	@echo "DEFINES              c defines, see src/kernel/flags.h"
	@echo "SETUP_ARGS           meson setup args, e.g. --optimization=3"
	@echo "COMPILE_ARGS         meson compile args, e.g. --verbose"
	@echo "TEST_ARGS            meson test args, e.g. --list"
	@echo "QEMU_BOOTARGS        qemu args, e.g. -append somebootargs"

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
		--cross-file ${CROSS_FILE_COMMON} \
		--cross-file ${CROSS_FILE_TARGET} \
		--cross-file ${CROSS_FILE_TOOLCHAIN} \
		--cross-file ${CROSS_FILE_FINAL} \
		--reconfigure \
		${ALL_SETUP_ARGS}

# meson compile
.PHONY: build compile
compile: build
kernel: build
build: setup
	${COMPILE_WRAPPER} meson compile -C ${BUILD} ${COMPILE_ARGS} ${JOBS}
# objdump kernels
# TODO: move into meson?
ifneq ($(strip ${DUMP}),)
	${OBJDUMP} -D ${KERNEL_ELF} > ${KERNEL_ELF}.objdump
	[ -f ${KERNEL_ELF_TEST} ] && ${OBJDUMP} -D ${KERNEL_ELF_TEST} > ${KERNEL_ELF_TEST}.objdump
endif

# meson test
.PHONY: test
test: build ${QEMU_FW}
	meson test -C ${BUILD} ${TEST_ARGS}

# run matrix test script
.PHONY: matrix-test
matrix-test:
	misc/run_test_matrix.sh

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
# only builds the required firmware
.PHONY: qemu run
run: qemu
qemu: build
	${QEMU_WRAP} ${QEMU_OUTFILE} ${QEMU} ${QEMU_FLAGS}

# generate the gdb init file
${GDB_INIT_FILE}:
	mkdir -p ${BUILD_BASE}
	sed 's/:PORT_GOES_HERE/:${GDB_PORT}/' < ${GDB_INIT_TEMPLATE} > $@

# run kernel via qemu and wait for a remote gdb to attach
# TODO: debug not working on sifive_u? maybe not supported?
.PHONY: qemu-gdb qemu-dbg
qemu-dbg: qemu-gdb
qemu-gdb: build ${GDB_INIT_FILE}
	@echo "gdb port is: ${GDB_PORT}"
	@echo "launch gdb with:"
	@echo
	@echo "gdb-multiarch -ix ${GDB_INIT_FILE} ${KERNEL_ELF}"
	@echo
	@echo "(CTRL+A X to exit)"
	${QEMU_WRAP} ${QEMU_OUTFILE} ${QEMU} ${QEMU_FLAGS} -S -gdb tcp::${GDB_PORT}

# dump and decode device tree
.PHONY: device-tree
device-tree: DEFINES += DUMP_DEVICE_TREE=1 EXAMPLE_PROCESSES_DISABLE=1
device-tree: build
	${QEMU_WRAP} ${QEMU_OUTFILE} ${QEMU} ${QEMU_FLAGS}
	${DEVICETREE_SCRIPT} ${QEMU_OUTFILE} ${BUILD}/dt.hex ${BUILD}/dt.dtb ${BUILD}/dt.dts
