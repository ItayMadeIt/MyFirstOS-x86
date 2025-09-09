#ifndef __INT_TIMER_H__
#define __INT_TIMER_H__

#include <stdint.h>
typedef struct int_timer_event 
{
    uint64_t timestamp_ns;
    uint64_t tick_count;
    uint32_t frequency_hz;
} int_timer_event_t;

typedef void (*int_timer_callback_t) (int_timer_event_t);

// Uses the interval timer to get the current amount of time
void init_int_timer(const uint64_t hz, int_timer_callback_t callback);
uint64_t int_timer_now_ns();
void int_timer_update_callback(int_timer_callback_t callback);
#endif // __INT_TIMER_H__