#include <core/defs.h>

struct irq_frame;
typedef struct irq_frame irq_frame_t;

void irq_enable();
void irq_disable();

uintptr_t irq_save();
void irq_restore(uintptr_t flags);

void irq_register_handler(uint32_t vector, void (*handle)(irq_frame_t*));
void irq_dispatch();

uintptr_t irq_frame_get_error(irq_frame_t* frame);

// callback = function called after