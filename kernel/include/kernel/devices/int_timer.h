#ifndef __INT_TIMER_H__
#define __INT_TIMER_H__

#include "core/num_defs.h"
typedef struct int_timer_event 
{
    u64 timestamp_ns;
    u64 tick_count;
    u32 frequency_hz;
} int_timer_event_t;

typedef void (*int_timer_callback_t) (int_timer_event_t);

// Uses the interval timer to get the current amount of time
void init_int_timer(const u64 hz, int_timer_callback_t callback);
u64 int_timer_now_ns();
void pit_update_callback(int_timer_callback_t callback);
#endif // __INT_TIMER_H__