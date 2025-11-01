#include <core/num_defs.h>

#define EARLY_TEXT_SECTION __attribute__((section(".text.boot"), noinline, used))
#define EARLY_BSS_SECTION  __attribute__((section(".bss.boot")))
#define EARLY_DATA_SECTION  __attribute__((section(".data.boot")))
