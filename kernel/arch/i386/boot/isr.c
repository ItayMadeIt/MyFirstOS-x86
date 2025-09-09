#include <stdint.h>
#include <early/defs.h>
#include <core/debug.h>
#include <kernel/core/cpu.h>
#include <kernel/interrupts/irq.h>
#include <arch/i386/boot/idt.h>

#define DIVIDE_0_INDEX   0
#define GEN_PROT_INDEX   13
#define PAGE_FAULT_INDEX 14

EARLY_TEXT_SECTION
void interrupt_div0(uint32_t _unused_)
{
    (void)_unused_;
    
    debug_print_str("Divide by 0 error\n");

    cpu_halt();
}

EARLY_TEXT_SECTION
void interrupt_gen_prot(uint32_t error)
{
    (void)error;

    debug_print_str("General Protection Fault\n");

    cpu_halt();
}

EARLY_TEXT_SECTION
void interrupt_page_fault(uint32_t error)
{
    debug_print_str("Page fault error:\n");

    // write/read
    if (error & 0b10)
        debug_print_str("Write page error\n");
    else 
        debug_print_str("Read page error\n");

    if (error & 0b100)
        debug_print_str("CPL == 3\n");
    else
        debug_print_str("CPL != 3\n");

    cpu_halt();
}

EARLY_TEXT_SECTION
void init_boot_isr()
{
    early_set_interrupt_c_callback(DIVIDE_0_INDEX  , interrupt_div0      );
    early_set_interrupt_c_callback(GEN_PROT_INDEX  , interrupt_gen_prot);
    early_set_interrupt_c_callback(PAGE_FAULT_INDEX, interrupt_page_fault);
}