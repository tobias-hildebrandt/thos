#pragma once

struct SbiReturn {
    long error;
    long value;
};
typedef struct SbiReturn SbiReturn;

SbiReturn sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4,
                   long arg5, long fid, long eid);

SbiReturn sbi_shutdown();
