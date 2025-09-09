#include <stdint.h>

#include <kernel/interrupts/irq.h>
#include <kernel/devices/int_timer.h>

#include <arch/i386/drivers/io/io.h>
#include <arch/i386/drivers/pic/pit.h>
#include <arch/i386/drivers/pic/pic.h>
#include <arch/i386/interrupts/irq.h>
#include <arch/i386/core/idt.h>

#define IRQ_TIMER_VEC   0x20 

// PIT constants
#define PIT_INPUT_CLOCK 3579545
#define PIT_DIVISOR     3
#define MIN_PIT_HZ 18
#define MAX_PIT_HZ 1193181

// ports
#define PIT_CHANNEL0    0x40
#define PIT_COMMAND     0x43

// pit globals
static uint32_t system_timer_ms_fractions = 0;
static uint32_t system_timer_ms = 0;
static uint32_t tick_ms_fractions = 0;
static uint32_t tick_ms = 0;
static uint32_t tick_ms_frequency = 0;
static uint16_t pit_reload_value = 0;
static uint32_t pit_hz = 0;
static uint32_t tick_count = 0;

static inline uint64_t div_round(uint64_t num, uint32_t div) 
{
    uint64_t q = num / div;
    uint64_t r = num % div;
    
    // round half up
    if (r * 2 >= div) 
        q++;
    
    return q;
}

static void compute_per_tick_time(uint32_t reload)
{
    // 64-bit product = reload * constant
    // constant = (3000 * 2^42) / 3579545
    uint64_t product = (uint64_t)reload * (uint64_t)0xDBB3A062;

    // divide by 2^10
    product >>= 10;

    // split into 32.32 fixed point
    tick_ms = (uint32_t)(product >> 32);
    tick_ms_fractions = (uint32_t)(product & 0xFFFFFFFFu);
}

static uint32_t compute_tick_frequency(uint32_t reload)
{
    uint64_t tmp = PIT_INPUT_CLOCK;

    uint32_t q = div_round(tmp, reload);

    uint32_t hz = div_round(q, PIT_DIVISOR);

    return hz;
}

static inline uint64_t compute_system_nanoseconds(void)
{
    uint64_t ns = (uint64_t)system_timer_ms * 1000000ULL;
    uint64_t frac_ns = ((uint64_t)system_timer_ms_fractions * 1000000ULL) >> 32;

    return ns + frac_ns;
}

extern void IRQ0_handler();

void pit_isr();
int_timer_callback_t pit_callback;

void set_pit_vars(uint32_t desired_hz)
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
        uint64_t tmp = PIT_INPUT_CLOCK;
        uint32_t div = desired_hz;

        uint64_t q = div_round(tmp, div);

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

void load_pit(uint32_t desired_hz)
{
    set_pit_vars(desired_hz);
    
    // Program PIT channel 0, mode 2 (rate generator)
    outb(PIT_COMMAND, 0x34); // ch0, lobyte/hibyte, mode 2
    outb(PIT_CHANNEL0, (uint8_t)(pit_reload_value & 0xFF));       // low
    outb(PIT_CHANNEL0, (uint8_t)((pit_reload_value >> 8) & 0xFF));// high
}


static void pit_timer_dispatch(irq_frame_t* frame) 
{
    (void)frame;

    uint64_t sum = (uint64_t)system_timer_ms_fractions + (uint64_t)tick_ms_fractions;
    system_timer_ms_fractions = (uint32_t)(sum & 0xFFFFFFFF);
    system_timer_ms += tick_ms + (uint32_t)(sum >> 32);

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

void init_int_timer(const uint64_t hz, int_timer_callback_t callback)
{
    load_pit(hz);

    system_timer_ms = 0;
    system_timer_ms_fractions = 0;
    tick_count = 0;
    pit_hz = hz;

    int_timer_update_callback(callback);

    irq_register_handler(
        IRQ_TIMER_VEC, 
        pit_timer_dispatch
    );

    // unmask IRQ0
    io_wait();
    outb(PIC1_DATA, inb(PIC1_DATA) & ~0x01);
}

void int_timer_update_callback(int_timer_callback_t callback) 
{
    assert(callback);
    pit_callback = callback;
}
