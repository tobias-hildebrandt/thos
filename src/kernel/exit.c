
#include <stdlib.h>

#include "flags.h"
#include "sbi.h"
#include "sifive_test.h"

// implement's stdlib.h's exit
void exit(int exit_code) {
    while (1) {
        if (EXIT_VIA_SBI) {
            sbi_shutdown();
        } else {
            sifive_exit(exit_code);
        }
    }
}
