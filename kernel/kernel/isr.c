#include <stdint.h>

void interrupt0_div()
{
    debug_print_str("Divide by 0 error\n");
    while(1);
}

void setup_isr()
{
    set_interrupt_callback(0, interrupt0_div);
}