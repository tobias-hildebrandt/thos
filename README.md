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
`thos` uses `make` as its build system.

- `make` builds the OS to `build/kernel.elf`
- `make run` (or `make qemu`) runs the OS on qemu
- `make qemu-gdb` (or `make qemu-dbg`) runs the OS on qemu but waits for gdb


## Target Details
- `rv64g`/`rv32g` architecture with `medany` code model
- `qemu-system-riscv64`/`qemu-system-riscv32` emulation
  - `virt` machine
  - `default` bios (OpenSBI)
