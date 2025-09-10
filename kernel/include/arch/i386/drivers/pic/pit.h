#include <core/defs.h>
#include <kernel/interrupts/irq.h>
#include <kernel/devices/int_timer.h>

#define IRQ_TIMER_VEC   0x20 

extern int_timer_callback_t pit_callback;
void pit_timer_dispatch(irq_frame_t* frame) ;
void load_pit(uint32_t desired_hz);
void set_pit_vars(uint32_t desired_hz);
void pit_update_callback(int_timer_callback_t callback);
