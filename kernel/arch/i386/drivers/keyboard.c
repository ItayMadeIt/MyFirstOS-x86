#include <arch/i386/drivers/pic/pic.h>
#include <arch/i386/drivers/pic/ps2_key.h>
#include <kernel/devices/keyboard.h>

void init_keyboard(key_event_cb_t callback)
{
    assert(callback);

    ps2_init_keycodes();

    irq_register_handler(
        PS2_IRQ_VECTOR,
        ps2_key_dispatch
    );

    key_event_callback = callback;

    unmask_ps2_key();
}
