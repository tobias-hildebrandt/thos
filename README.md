# thos
`thos` is a RISC-V hobby OS targeting qemu, written in C and inline assembly.

## Current Features
- Single core
- Kernel and user processes
- Supports qemu's `virt` and `sifive_u` machines
- 64-bit and 32-bit support
- Virtual memory (Sv39 or Sv32)
- Full context switching
- Software interrupts, timer interrupts, and syscalls
- Partial C standard library

## Build System
`thos` uses `meson` as its build system and provides a `make` wrapper.
See `make help`.

- `make build` configures and builds the kernel
- `make qemu` runs the OS on qemu
- `make qemu-gdb` runs the OS on qemu but waits for gdb
- `make test` builds a test kernel and runs all tests on qemu
