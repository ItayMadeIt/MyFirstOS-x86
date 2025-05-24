#include <stdint.h>
#include <stdbool.h>
#include <drivers/io.h>
#include <drivers/isr.h>
#include <core/idt.h>
#include <core/gdt.h>
#include <multiboot/multiboot.h>

extern uint8_t kernel_end;

void halt() 
{
    asm volatile (
        "cli\n\t"   // disable interrupts
        "hlt\n\t"   // halt the CPU
    );
}


void virt_kernel_main()
{
    asm volatile (
        "mov $kernel_main, %%eax\n\t"
        "add $0xBFF00000, %%eax\n\t"
        "call *%%eax\n\t"
        : 
        : 
        : "eax", "memory"
    );
}

static inline void sti()
{
    asm volatile("sti");
}

static inline int interrupts_enabled(void) 
{
    unsigned long eflags;

    asm volatile (
        "pushf\n\t"
        "pop %0"
        : "=r"(eflags)
    );
    
    return (eflags & (1 << 9)) ? 1 : 0;
}



void entry_main(uint32_t magic, multiboot_info_t* mbd)
{
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        debug_print_str("Magic value wasn't present after boot.\n");
        halt();
    }

    // Critical setup
    setup_gdt();
    setup_idt();
    
    // Setup phys allocator
    setup_phys_allocator(mbd);

    // Setup safety net
    setup_paging();
    
    // Setup interrutp handlers
    setup_isr();

    // Setup inputs using pic1, pic2
    setup_pic();
    
    // Setup basic drivers
    setup_pit();
    setup_ps2();

    sti();

    while(true);

    virt_kernel_main();
}