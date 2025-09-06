#include <stdint.h>
#include <arch/irq.h>
#include <stdio.h>
#include <arch/i386/drivers/pit.h>

#define IRQ_TIMER 0x20

extern void IRQ0_handler();

void pit_isr();
void (* pit_callback);

static void dummy() 
{
    static uint64_t last_system_timer = ~0;

    if (last_system_timer != pit_system_timer_ms / 1000)
    {
        printf("%d\n", pit_system_timer_ms/1000);
        last_system_timer =  pit_system_timer_ms/1000;
    }
}

void int_timer_set_callback(void (*func)())
{
    pit_callback = func;
}

void init_int_timer(const uint64_t hz)
{
    pit_system_timer_ms = 0;
    pit_system_timer_ms_fractions = 0;
    pit_callback = dummy;

    irq_register_handler(IRQ_TIMER, pit_isr);

    asm volatile (
        "mov %0, %%ebx\n\r"
        "call setup_pit_asm\n\r"
        :
        : "r" (hz)
        : "memory", "eax", "ebx"
    );
}
