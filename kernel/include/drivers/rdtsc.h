#include <core/defs.h>

static inline uint64_t rdtsc()
{
    uint32_t lo, hi;
    
    asm volatile(
        "rdtsc"
        : "=a"(lo), "=d"(hi)
    );

    return ((uint64_t)hi << 32) | lo;
} 

static inline uint64_t rdtsc_serialized(void) 
{
    uint32_t hi, lo;
    asm volatile (
        "lfence\nrdtsc" 
        : "=a"(lo), "=d"(hi) 
        :
        : "memory"
    );

    return ((uint64_t)hi << 32) | lo;
}

void calibrate_rdtsc();

extern uint32_t rdtsc_ps_per_tick;

static inline uint64_t get_rdtsc_time()
{
    uint64_t t = rdtsc_serialized();
    return (t * rdtsc_ps_per_tick) / 1000ULL;
}