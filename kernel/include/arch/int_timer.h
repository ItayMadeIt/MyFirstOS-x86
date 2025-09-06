#include <stdint.h>

// Uses the interval timer to get the current amount of time
void init_int_timer(const uint64_t hz);
uint64_t int_timer_now_ns();
void int_timer_set_callback(void (*func)());