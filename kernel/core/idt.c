#include <stdint.h>
#include <stdbool.h>
#include <core/idt.h>

idt_entry_t idt_entries[IDT_ENTRIES] __attribute__((aligned(16)));
void (*interrupt_callback_entries[IDT_ENTRIES]) (uint32_t error_code);

void idt_c_handler(uint8_t interrupt_number, uint32_t error_code)
{
    return;
    
    if (interrupt_number >= 32)
    {
        debug_print_str("Interrupt number wasn't normal\n\n");
        debug_print(interrupt_number);

        // Sort of a halt
        interrupt_callback_entries[interrupt_number](error_code);
    }

    debug_print_str("Interrupt number is: ");
    debug_print(interrupt_number);

    interrupt_callback_entries[interrupt_number](error_code);
}

void set_idt_entry(uint32_t entry_index, void (*handler_addr), uint16_t selector, uint8_t type_attr)
{
    idt_entries[entry_index].zero = 0;
    
    idt_entries[entry_index].offset_low = ((uint32_t)handler_addr) & 0xFFFF;
    idt_entries[entry_index].offset_high = ((uint32_t)handler_addr >> 16) & 0xFFFF;

    idt_entries[entry_index].segment_selector = selector;
    idt_entries[entry_index].type_attr = type_attr;
}


void set_idt_callback(uint16_t index, void (*handler_addr))
{
    idt_entries[index].offset_low = ((uint32_t)handler_addr) & 0xFFFF;
    idt_entries[index].offset_high = ((uint32_t)handler_addr >> 16) & 0xFFFF;
}

static inline void load_idt_descriptor(idt_descriptor_t* idt_descriptor)
{
    asm volatile (
        "lidt (%0)"
        :
        : "r" (idt_descriptor)
        : "memory"
    );
}

void set_interrupt_c_callback(uint8_t entry_index, void (*callback) (uint32_t error_code))
{
    interrupt_callback_entries[entry_index] = callback;
}

void setup_idt()
{
    for (uint16_t i = 0; i < IDT_ENTRIES; ++i) 
    {
        interrupt_callback_entries[i] = 0;
    
        set_idt_entry(
            i, 
            idt_c_handler, 
            SEGMENT_SELECTOR_CODE_DPL0, 
            IDT_INTERRUPT_32_DPL0
        );
    }

    set_idt_entry(0x00, isr0 , SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x01, isr1 , SEGMENT_SELECTOR_CODE_DPL0, IDT_TRAP_32_PL0);
    set_idt_entry(0x02, isr2 , SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x03, isr3 , SEGMENT_SELECTOR_CODE_DPL0, IDT_TRAP_32_PL0);
    set_idt_entry(0x04, isr4 , SEGMENT_SELECTOR_CODE_DPL0, IDT_TRAP_32_PL0);
    set_idt_entry(0x05, isr5 , SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x06, isr6 , SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x07, isr7 , SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x08, isr8 , SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x09, isr9 , SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x0A, isr10, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x0B, isr11, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x0C, isr12, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x0D, isr13, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x0E, isr14, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x0F, isr15, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x10, isr16, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x11, isr17, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x12, isr18, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x13, isr19, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x14, isr20, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x15, isr21, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x16, isr22, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x17, isr23, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x18, isr24, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x19, isr25, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x1A, isr26, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x1B, isr27, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x1C, isr28, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x1D, isr29, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x1E, isr30, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x1F, isr31, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);

    idt_descriptor_t idt_descriptor;
    idt_descriptor.base = (uint32_t)&idt_entries;
    idt_descriptor.limit = sizeof(idt_entries)- 1;

    load_idt_descriptor(&idt_descriptor);
}
