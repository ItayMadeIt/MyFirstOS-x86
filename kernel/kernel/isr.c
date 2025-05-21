#include <stdint.h>

#define DIVIDE_0_INDEX 0
#define PAGE_FAULT_INDEX 14

void interrupt_div0(uint32_t _unused_)
{
    debug_print_str("Divide by 0 error\n");

    halt();
}

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

    halt();
}

void setup_isr()
{
    set_interrupt_callback(DIVIDE_0_INDEX  , interrupt_div0      );
    set_interrupt_callback(PAGE_FAULT_INDEX, interrupt_page_fault);
}