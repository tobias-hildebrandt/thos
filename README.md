# thos
`thos` is a 64-bit RISC-V hobby OS targeting qemu, written in C and inline assembly.


## Current Features
- Single core
- Kernel and user processes
- Virtual memory (Sv39)
- Context switching through software interrupts
- Partial C standard library


## Build System
`thos` uses `make` as its build system.

- `make` builds the OS to `build/kernel.elf`
- `make run` (or `make qemu`) runs the OS on qemu
- `make qemu-gdb` (or `make qemu-dbg`) runs the OS on qemu but waits for gdb


## Target Details
- `rv64g` architecture with `medany` code model
- `qemu-system-riscv64` emulation
  - `virt` machine
  - `default` bios (OpenSBI)
