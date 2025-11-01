#include <core/defs.h>

struct irq_frame;
typedef struct irq_frame irq_frame_t;

void irq_enable();
void irq_disable();

bool irq_is_enabled();

usize_ptr irq_save();
void irq_restore(usize_ptr flags);

void irq_register_handler(u32 vector, void (*handle)(irq_frame_t*));
void irq_dispatch();

usize_ptr irq_frame_get_error(irq_frame_t* frame);

// callback = function called after