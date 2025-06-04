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
    
    setup_phys_allocator(mbd);

    setup_paging();

    // Setup interrupt handlers
    setup_isr();

    setup_memory();

    // inputs using pic1, pic2
    setup_pic();

    // basic drivers
    setup_pit();
    setup_ps2();

    sti();

    virt_kernel_main();
}