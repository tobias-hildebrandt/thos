#pragma once

#define SECTION(x) extern char x[]

SECTION(__KERNEL_BASE);
SECTION(__PAGES_START);
SECTION(__PAGES_END);
