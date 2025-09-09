#include <kernel/interrupts/irq.h>
#include <kernel/devices/keyboard.h>

extern key_event_cb_t key_event_callback;;

#define PS2_DATA 0x60
#define PS2_CMD 0x64

#define PS2_IRQ_VECTOR 0x21

void unmask_ps2_key();
void ps2_init_keycodes();
void ps2_key_dispatch(irq_frame_t* frame);

