#include "core/abort.h"
#include "kernel/interrupts/irq.h"
#include "kernel/core/cpu.h"

void abort()
{
    // for now
    irq_disable();
    cpu_halt();
}
