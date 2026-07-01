#pragma once

struct SbiReturn {
    long error;
    long value;
};
typedef struct SbiReturn SbiReturn;

// USE MACRO!
SbiReturn __sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                     long arg5, long fid, long eid);

#define __SBI_EXPAND(x) x
#define __SBI_GET_MACRO(_eid, _fid, _arg0, _arg1, _arg2, _arg3, _arg4, _arg5, \
                        name, ...)                                            \
    name
#define SBI_CALL(...)                                                          \
    __SBI_EXPAND(__SBI_GET_MACRO(__VA_ARGS__, SBI_CALL6, SBI_CALL5, SBI_CALL4, \
                                 SBI_CALL3, SBI_CALL2, SBI_CALL1,              \
                                 SBI_CALL0)(__VA_ARGS__))

#define SBI_CALL0(eid, fid) __sbi_call(0, 0, 0, 0, 0, fid, eid)
#define SBI_CALL1(eid, fid, arg0) __sbi_call(arg0, 0, 0, 0, 0, 0, fid, eid)
#define SBI_CALL2(eid, fid, arg0, arg1) \
    __sbi_call(arg0, arg1, 0, 0, 0, 0, fid, eid)
#define SBI_CALL3(eid, fid, arg0, arg1, arg2) \
    __sbi_call(arg0, arg1, arg2, 0, 0, 0, fid, eid)
#define SBI_CALL4(eid, fid, arg0, arg1, arg2, arg3) \
    __sbi_call(arg0, arg1, arg2, arg3, 0, 0, fid, eid)
#define SBI_CALL5(eid, fid, arg0, arg1, arg2, arg3, arg4) \
    __sbi_call(arg0, arg1, arg2, arg3, arg4, 0, fid, eid)
#define SBI_CALL6(eid, fid, arg0, arg1, arg2, arg3, arg4, arg5) \
    __sbi_call(arg0, arg1, arg2, arg3, arg4, arg5, fid, eid)

SbiReturn sbi_putchar(char c);
SbiReturn sbi_shutdown(void);
