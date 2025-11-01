#include <arch/i386/firmware/multiboot/multiboot.h>
#include "core/num_defs.h"

// constant offset: virt = phys + off
extern char __va_pa_off[];
extern char kernel_stack_top[];

void entry_main(u32 magic, multiboot_info_t* mbd);

void init_gdt();
void init_boot_isr();
void init_paging();

static inline uptr virt_to_phys_code(uptr virt)
{
    return virt - (uptr)__va_pa_off;
}

static inline void* phys_to_virt_code(uptr phys)
{
    return (void*)(phys + __va_pa_off);
}
