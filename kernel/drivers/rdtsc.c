#include <stdint.h>
#include <boot/acpi.h>

#define ACPI_HZ 3579545u

static inline uint64_t rdtsc_serialized(void) 
{
    unsigned hi, lo;
    __asm__ volatile ("lfence\nrdtsc" : "=a"(lo), "=d"(hi) :: "memory");
    return ((uint64_t)hi << 32) | lo;
}

uint32_t rdtsc_ps_per_tick;

void calibrate_rdtsc(void)
{
    const uint32_t width = (acpi_timer.flags & ACPI_TIMER_24) ? 24u : 32u;
    const uint32_t mask  = (width == 24) ? ((1u << 24) - 1u) : 0xFFFFFFFFu;
    const uint32_t target_ticks = ACPI_HZ / 100; // ~10 ms

    uint32_t a0 = get_acpi_time() & mask;
    uint64_t t0 = rdtsc_serialized();

    // wait 10 ms by ACPI
    while (1) 
    {
        uint32_t now = get_acpi_time() & mask;
        uint32_t delta = (now - a0) & mask;
        
        if (delta >= target_ticks) 
            break;
    }

    uint64_t t1 = rdtsc_serialized();

    uint64_t rdtsc_delta = t1 - t0;
    uint32_t acpi_delta  = (get_acpi_time() - a0) & mask;
    if (!rdtsc_delta || !acpi_delta) 
    { 
        rdtsc_ps_per_tick = 0; 
        return; 
    }

    // TSC frequency in Hz
    uint64_t tsc_freq_hz = (rdtsc_delta * (uint64_t)ACPI_HZ) / acpi_delta;

    // ps per tick = 1e12 / Hz (rounded down)
    rdtsc_ps_per_tick = 
        (tsc_freq_hz ? (uint32_t)(1000000000000ULL / tsc_freq_hz) : 0);
}
