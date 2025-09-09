#include <arch/i386/firmware/multiboot/multiboot.h>

void entry_main(uint32_t magic, multiboot_info_t* mbd);

void init_gdt();
void init_boot_isr();
void init_paging();