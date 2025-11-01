#include "core/num_defs.h"
#include <early/defs.h>
#include <kernel/core/cpu.h>
#include <kernel/interrupts/irq.h>
#include <arch/i386/boot/idt.h>
#include "arch/i386/boot/early_tty.h"

#define DIVIDE_0_INDEX   0
#define GEN_PROT_INDEX   13
#define PAGE_FAULT_INDEX 14

EARLY_TEXT_SECTION
void interrupt_div0(u32 _unused_)
{
    (void)_unused_;
    
    early_printf("Divide by 0 error\n");

    cpu_halt();
}

EARLY_TEXT_SECTION
void interrupt_gen_prot(u32 error)
{
    (void)error;

    early_printf("General Protection Fault\n");

    cpu_halt();
}

EARLY_TEXT_SECTION
void interrupt_page_fault(u32 error)
{
    early_printf("Page fault error:\n");

    // write/read
    if (error & 0b10)
        early_printf("Write page error\n");
    else 
        early_printf("Read page error\n");

    if (error & 0b100)
        early_printf("CPL == 3\n");
    else
        early_printf("CPL != 3\n");

    cpu_halt();
}

EARLY_TEXT_SECTION
void init_boot_isr()
{
    early_set_interrupt_c_callback(DIVIDE_0_INDEX  , interrupt_div0      );
    early_set_interrupt_c_callback(GEN_PROT_INDEX  , interrupt_gen_prot);
    early_set_interrupt_c_callback(PAGE_FAULT_INDEX, interrupt_page_fault);
}