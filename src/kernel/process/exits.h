#pragma once

typedef void(ProcessExitFunc)(void);

ProcessExitFunc ProcessExit_user;
ProcessExitFunc ProcessExit_kernel;
