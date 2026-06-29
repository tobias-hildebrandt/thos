#pragma once

// macros for asm string creation
#define REGISTER_MEM(instr, base, reg, offset) \
    #instr " " #reg ", 8 * (" #offset ")(" #base ")\n"
#define REGISTER_MEM_PREFIX(instr, base, start_offset, prefix, x) \
    REGISTER_MEM(instr, base, prefix##x, start_offset + x)
