#include "virtual_memory/satp.h"

#include <stdint.h>

#include "sections.h"
#include "virtual_memory/page.h"
#include "virtual_memory/page_table.h"

IN_GLOBAL_SPECIAL SatpRegister kernel_page_satp = {0};

SatpRegister SatpRegister_from_PageTable(PageTable table) {
    SatpRegister satp_register = {0};

    satp_register.mode = SATP_MODE;
    satp_register.physical_page_num = (uintptr_t)table / PAGE_SIZE;

    return satp_register;
}
