# thos
`thos` is a RISC-V hobby OS targeting qemu, written in C and inline assembly.

## Current Features
- Single core
- Kernel and user processes
- Supports 64-bit or 32-bit target
- Virtual memory (Sv39 in 64-bit mode, Sv32 in 32-bit mode)
- Context switching through software interrupts
- Partial C standard library

## Build System
`thos` uses `meson` as its build system and provides a `make` wrapper.

- `make` or `make kernel` configures and builds the kernel
- `make run` (or `make qemu`) runs the OS on qemu
- `make qemu-gdb` (or `make qemu-dbg`) runs the OS on qemu but waits for gdb
- `make test` builds a test kernel and runs all tests on qemu

## Target Details
- `rv64g`/`rv32g` architecture with `medany` code model
- `qemu-system-riscv64`/`qemu-system-riscv32` emulation
  - `virt` machine
  - `default` bios (OpenSBI)
