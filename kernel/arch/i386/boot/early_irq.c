#include <early/defs.h>

EARLY_TEXT_SECTION
void early_irq_disable()
{
    asm volatile("cli\n\r");
}

EARLY_TEXT_SECTION
void early_irq_enable()
{
    asm volatile("sti\n\r");
}