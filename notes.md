# compile-time define flags
- `./misc/find_compile_flags.sh`
- pass to make via `make KERNEL_CFLAGS_EXTRA="-DSOMETHING -DANOTHER -DXXXXX"`
  - (probably will need to `make clean` before or just call with `-B`)

#
- `riscv64-unknown-elf-objdump -d build/kernel.elf`
- `riscv64-unknown-elf-addr2line <addr> -e build/kernel.elf`
- `riscv64-unknown-elf-nm build/kernel.elf`
#
- https://operating-system-in-1000-lines.vercel.app/en/
- https://riscv.org/wp-content/uploads/2024/12/riscv-calling.pdf
- https://lists.riscv.org/g/tech-psabi/attachment/61/0/riscv-abi.pdf
- https://docs.riscv.org/reference/home/index.html
- https://github.com/mit-pdos/xv6-riscv/blob/riscv/Makefile
- https://wiki.osdev.org/Expanded_Main_Page
- https://sourceware.org/binutils/docs/ld/
- https://makefiletutorial.com/
- https://darkdust.net/files/GDB%20Cheat%20Sheet.pdf
- https://lupyuen.org/articles/sbi
- https://github.com/antirez/sds
- https://gcc.gnu.org/onlinedocs/gcc/Using-Assembly-Language-with-C.html
- https://xv6-guide.github.io/xv6-riscv-book/index.html
- https://lupyuen.org/articles/mmu
- https://osdev.wiki/wiki/RISC-V_Paging
- https://sourceware.org/binutils/docs/as/
- https://github.com/riscv-software-src/opensbi/blob/master/docs/platform/qemu_virt.md
