#include <stdint.h>
#include <core/idt.h>

void IRQ0_handler();

void setup_pit()
{
    set_idt_entry(
        0x20, 
        IRQ0_handler, 
        SEGMENT_SELECTOR_CODE_DPL0, 
        IDT_INTERRUPT_32_DPL0
    );

    const uint32_t frequency_hz = 200; // 200 Hz -> every 5 ms

    asm volatile (
        "mov %0, %%ebx\n\r"
        "call setup_pit_asm\n\r"
        :
        : "r" (frequency_hz)
        : "memory", "eax", "ebx"
    );
}
