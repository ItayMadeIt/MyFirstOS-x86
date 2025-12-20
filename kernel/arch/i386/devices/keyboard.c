#include <arch/i386/drivers/pic/pic.h>
#include <arch/i386/drivers/pic/ps2_key.h>
#include <kernel/devices/keyboard.h>
#include "core/assert.h"

void init_keyboard(key_event_cb_t callback)
{
    assert(callback);

    ps2_init();

    irq_register_handler(
        PS2_IRQ_VECTOR,
        ps2_key_dispatch
    );

    pic_unmask_vector(PS2_IRQ_VECTOR);

    key_event_callback = callback;
}
