#include "core/num_defs.h"
#include "core/assert.h"

#include <kernel/interrupts/irq.h>
#include <kernel/devices/int_timer.h>

#include <arch/i386/drivers/io/io.h>
#include <arch/i386/drivers/pic/pit.h>
#include <arch/i386/drivers/pic/pic.h>
#include <arch/i386/interrupts/irq.h>
#include <arch/i386/core/idt.h>


// PIT constants
#define PIT_INPUT_CLOCK 3579545
#define PIT_DIVISOR     3
#define MIN_PIT_HZ 18
#define MAX_PIT_HZ 1193181

// ports
#define PIT_CHANNEL0    0x40
#define PIT_COMMAND     0x43

// pit globals
static u32 system_timer_ms_fractions = 0;
static u32 system_timer_ms = 0;
static u32 tick_ms_fractions = 0;
static u32 tick_ms = 0;
static u32 tick_ms_frequency = 0;
static u16 pit_reload_value = 0;
static u32 pit_hz = 0;
static u32 tick_count = 0;

static inline u64 div_round(u64 num, u32 div) 
{
    u64 q = num / div;
    u64 r = num % div;
    
    // round half up
    if (r * 2 >= div) 
        q++;
    
    return q;
}

static void compute_per_tick_time(u32 reload)
{
    // 64-bit product = reload * constant
    // constant = (3000 * 2^42) / 3579545
    u64 product = (u64)reload * (u64)0xDBB3A062;

    // divide by 2^10
    product >>= 10;

    // split into 32.32 fixed point
    tick_ms = (u32)(product >> 32);
    tick_ms_fractions = (u32)(product & 0xFFFFFFFFu);
}

static u32 compute_tick_frequency(u32 reload)
{
    u64 tmp = PIT_INPUT_CLOCK;

    u32 q = div_round(tmp, reload);

    u32 hz = div_round(q, PIT_DIVISOR);

    return hz;
}

static inline u64 compute_system_nanoseconds(void)
{
    u64 ns = (u64)system_timer_ms * 1000000ULL;
    u64 frac_ns = ((u64)system_timer_ms_fractions * 1000000ULL) >> 32;

    return ns + frac_ns;
}

int_timer_callback_t pit_callback;

void set_pit_vars(u32 desired_hz)
{
    // Clamp reload value (too slow or too fast)
    if (desired_hz <= MIN_PIT_HZ) 
    {
        pit_reload_value = 0xFFFF;
    } 
    else if (desired_hz >= MAX_PIT_HZ) 
    {
        pit_reload_value = 1;
    } 
    else 
    {
        // Compute reload (PIT_INPUT_CLOCK / desired_hz) / 3
        u64 tmp = PIT_INPUT_CLOCK;
        u32 div = desired_hz;

        u64 q = div_round(tmp, div);

        // Divide by 3 with rounding
        pit_reload_value = div_round(q, PIT_DIVISOR);

        if (pit_reload_value == 0)
            pit_reload_value = 1;
    }

    // Compute corrected frequency: (3579545 / reload) / 3
    tick_ms_frequency = compute_tick_frequency(pit_reload_value);

    // Calculate ms per tick in fixed-point
    compute_per_tick_time(pit_reload_value);
}

void load_pit(u32 desired_hz)
{
    set_pit_vars(desired_hz);
    
    // Program PIT channel 0, mode 2 (rate generator)
    outb(PIT_COMMAND, 0x34); // ch0, lobyte/hibyte, mode 2
    outb(PIT_CHANNEL0, (u8)(pit_reload_value & 0xFF));       // low
    outb(PIT_CHANNEL0, (u8)((pit_reload_value >> 8) & 0xFF));// high

    // unmask IRQ0
    pic_unmask_vector(IRQ_TIMER_VEC);
}

void pit_timer_dispatch(irq_frame_t* frame) 
{
    (void)frame;

    u64 sum = (u64)system_timer_ms_fractions + (u64)tick_ms_fractions;
    system_timer_ms_fractions = (u32)(sum & 0xFFFFFFFF);
    system_timer_ms += tick_ms + (u32)(sum >> 32);

    tick_count++;

    if (pit_callback)
    {
        int_timer_event_t event = {
            .timestamp_ns=compute_system_nanoseconds(),
            .tick_count = tick_count,
            .frequency_hz = pit_hz,
        };

        pit_callback(event);
    }

    pic_send_eoi_vector(IRQ_TIMER_VEC);
}

void pit_update_callback(int_timer_callback_t callback) 
{
    assert(callback);
    pit_callback = callback;
}
