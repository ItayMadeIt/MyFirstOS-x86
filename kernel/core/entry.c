#include <stdint.h>
#include <stdbool.h>
#include <drivers/io.h>
#include <drivers/isr.h>
#include <core/idt.h>
#include <core/gdt.h>

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

void IRQ0_handler();

void setup_pit()
{
    set_idt_entry(0x20, IRQ0_handler, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);

    const uint32_t frequency = 1000; // 1000 Hz

    asm volatile (
        "mov %0, %%ebx\n\r"
        "call setup_pit_asm\n\r"
        :
        : "r" (frequency)
        : "memory"
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



void entry_main()
{
    // Critical setup
    setup_gdt();
    setup_idt();
    setup_paging();
    
    // Setup interrutp handlers
    setup_isr();

    // Setup inputs using pic1, pic2
    setup_pic();
    
    // Setup basic one-core timer
    setup_pit();

    sti();

    while(1);

    //virt_kernel_main();
}