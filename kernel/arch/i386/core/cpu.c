#include <kernel/core/cpu.h>
#include <arch/i386/interrupts/irq.h>
#include <arch/i386/drivers/pic/pic.h>

void cpu_halt()
{
    asm volatile(
        "hlt\n\t"
    ); 
}

void cpu_relax()
{
    asm volatile(
        "pause\n"
        : 
        : 
        :"memory"
    );
}

static inline void enable_wp(void) 
{
    u32 cr0;
    asm volatile ("mov %%cr0,%0" : "=r"(cr0));
    cr0 |= (1u << 16);              // CR0.WP
    asm volatile ("mov %0,%%cr0" :: "r"(cr0) : "memory");
}

void cpu_init()
{
    init_irq();

    enable_wp();

    setup_pic();
}