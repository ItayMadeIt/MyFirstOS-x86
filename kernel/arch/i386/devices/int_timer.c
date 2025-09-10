#include <kernel/devices/int_timer.h>

#include <arch/i386/drivers/pic/pit.h>

void init_int_timer(const uint64_t hz, int_timer_callback_t callback)
{
    load_pit(hz);

    pit_update_callback(callback);

    irq_register_handler(
        IRQ_TIMER_VEC, 
        pit_timer_dispatch
    );
}