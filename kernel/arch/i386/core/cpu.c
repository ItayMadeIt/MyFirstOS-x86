#include <kernel/core/cpu.h>
#include <arch/i386/core/irq.h>
#include <arch/i386/drivers/pic.h>

void cpu_halt()
{
    asm volatile(
        "cli\n\t"
        "hlt\n\t"
    ); 
}

void cpu_init()
{
    init_irq();

    setup_pic();
    
    // create dummy irq functions to see if they are getting called
}