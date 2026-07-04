#pragma once

long syscall(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5,
             long arg6, long arg7);

#define __SYSCALL_EXPAND(x) x
#define __SYSCALL_GET_MACRO(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, \
                            _arg7, name, ...)                                \
    name
#define SYSCALL(...)                                                   \
    __SYSCALL_EXPAND(__SYSCALL_GET_MACRO(                              \
        __VA_ARGS__, SYSCALL8, SYSCALL7, SYSCALL6, SYSCALL5, SYSCALL4, \
        SYSCALL3, SYSCALL2, SYSCALL1, __FILLER)(__VA_ARGS__))

// clang-format off
#define SYSCALL1(arg0) \
         syscall(arg0, 0, 0, 0, 0, 0, 0, 0)
#define SYSCALL2(arg0, arg1) \
         syscall(arg0, arg1, 0, 0, 0, 0, 0, 0)
#define SYSCALL3(arg0, arg1, arg2) \
         syscall(arg0, arg1, arg2, 0, 0, 0, 0, 0)
#define SYSCALL4(arg0, arg1, arg2, arg3) \
         syscall(arg0, arg1, arg2, arg3, 0, 0, 0, 0)
#define SYSCALL5(arg0, arg1, arg2, arg3, arg4) \
         syscall(arg0, arg1, arg2, arg3, arg4, 0, 0, 0)
#define SYSCALL6(arg0, arg1, arg2, arg3, arg4, arg5) \
         syscall(arg0, arg1, arg2, arg3, arg4, arg5, 0, 0)
#define SYSCALL7(arg0, arg1, arg2, arg3, arg4, arg5, arg6) \
         syscall(arg0, arg1, arg2, arg3, arg4, arg5, arg6, 0)
#define SYSCALL8(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
         syscall(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
// clang-format on

// do_putchar implemented in user libc implementation

void yield(void);
