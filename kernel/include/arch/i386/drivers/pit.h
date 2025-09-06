#include <core/defs.h>

extern uint32_t pit_system_timer_ms_fractions;
extern uint32_t pit_system_timer_ms;
extern uint32_t IRQ0_fractions;
extern uint32_t IRQ0_ms;

extern void (* pit_callback);
void setup_pit_asm();