#include <core/defs.h>

void irq_enable();
void irq_disable();

uintptr_t irq_save();
void irq_restore(uintptr_t flags);

void irq_register_handler(uint32_t vector, void (*handle)(void*));
void irq_dispatch();

// callback = function called after