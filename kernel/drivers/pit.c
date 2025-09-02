#include <stdint.h>
#include <core/idt.h>
#include <drivers/pit.h>
#include <stdio.h>

extern void IRQ0_handler();

void (* pit_callback);

static void dummy() 
{
    printf("%X\n", system_timer_ms/1000);
}

void setup_pit()
{
    system_timer_ms = 0;
    system_timer_fractions = 0;
    pit_callback = dummy;

    set_idt_entry(
        0x20, 
        IRQ0_handler, 
        SEGMENT_SELECTOR_CODE_DPL0, 
        IDT_INTERRUPT_32_DPL0
    );

    const uint32_t frequency_hz = 20; // 100 Hz -> every 10 ms

    asm volatile (
        "mov %0, %%ebx\n\r"
        "call setup_pit_asm\n\r"
        :
        : "r" (frequency_hz)
        : "memory", "eax", "ebx"
    );
}
