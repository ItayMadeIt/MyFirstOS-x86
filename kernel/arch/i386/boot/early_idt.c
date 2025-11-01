// Not the real IDT, just a trampoline for only necessary interrupts that can't be modified
#include <core/defs.h>
#include <early/defs.h>
#include <arch/i386/boot/idt.h>
#include <arch/i386/boot/early_tty.h>

EARLY_BSS_SECTION
idt_entry_t early_idt_entries[IDT_ENTRIES];
EARLY_BSS_SECTION
void (*early_interrupt_callback_entries[IDT_ENTRIES]) (u32 error_code);

void early_idt_c_handler(u8 interrupt_number, u32 error_code)
{
    if (interrupt_number >= BASE_INTERRUPTS_ENTRIES)
    {
        early_printf("Interrupt number wasn't normal\n\n");
        early_printf("%u", (u32)interrupt_number);

        early_interrupt_callback_entries[interrupt_number](error_code);
    }

    early_printf("Interrupt number is: %u", (u32)interrupt_number);

    early_interrupt_callback_entries[interrupt_number](error_code);
}

EARLY_TEXT_SECTION
void early_set_idt_entry(u32 entry_index, void (*handler_addr), u16 selector, u8 type_attr)
{
    early_idt_entries[entry_index].zero = 0;
    
    early_idt_entries[entry_index].offset_low = ((u32)handler_addr) & 0xFFFF;
    early_idt_entries[entry_index].offset_high = ((u32)handler_addr >> 16) & 0xFFFF;

    early_idt_entries[entry_index].segment_selector = selector;
    early_idt_entries[entry_index].type_attr = type_attr;
}

EARLY_TEXT_SECTION
void early_set_idt_callback(u16 index, void (*handler_addr))
{
    early_idt_entries[index].offset_low = ((u32)handler_addr) & 0xFFFF;
    early_idt_entries[index].offset_high = ((u32)handler_addr >> 16) & 0xFFFF;
}

EARLY_TEXT_SECTION
static inline void early_load_idt_descriptor(idt_descriptor_t* idt_descriptor)
{
    asm volatile (
        "lidt (%0)"
        :
        : "r" (idt_descriptor)
        : "memory"
    );
}

EARLY_TEXT_SECTION
void early_set_interrupt_c_callback(u8 entry_index, void (*callback) (u32 error_code))
{
    early_interrupt_callback_entries[entry_index] = callback;
}

EARLY_TEXT_SECTION
void setup_early_idt()
{
    for (u16 i = 0; i < IDT_ENTRIES; ++i) 
    {
        early_interrupt_callback_entries[i] = 0;
    
        early_set_idt_entry(
            i,
            early_idt_c_handler, 
            SEGMENT_SELECTOR_CODE_DPL0, 
            IDT_INTERRUPT_32_DPL0
        );
    }

    early_set_idt_entry(0x00, early_isr0 , SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x01, early_isr1 , SEGMENT_SELECTOR_CODE_DPL0, IDT_TRAP_32_PL0);
    early_set_idt_entry(0x02, early_isr2 , SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x03, early_isr3 , SEGMENT_SELECTOR_CODE_DPL0, IDT_TRAP_32_PL0);
    early_set_idt_entry(0x04, early_isr4 , SEGMENT_SELECTOR_CODE_DPL0, IDT_TRAP_32_PL0);
    early_set_idt_entry(0x05, early_isr5 , SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x06, early_isr6 , SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x07, early_isr7 , SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x08, early_isr8 , SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x09, early_isr9 , SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x0A, early_isr10, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x0B, early_isr11, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x0C, early_isr12, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x0D, early_isr13, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x0E, early_isr14, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x0F, early_isr15, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x10, early_isr16, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x11, early_isr17, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x12, early_isr18, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x13, early_isr19, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x14, early_isr20, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x15, early_isr21, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x16, early_isr22, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x17, early_isr23, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x18, early_isr24, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x19, early_isr25, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x1A, early_isr26, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x1B, early_isr27, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x1C, early_isr28, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x1D, early_isr29, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x1E, early_isr30, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    early_set_idt_entry(0x1F, early_isr31, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);

    idt_descriptor_t idt_descriptor;
    idt_descriptor.base = (u32)&early_idt_entries;
    idt_descriptor.limit = sizeof(early_idt_entries)- 1;

    early_load_idt_descriptor(&idt_descriptor);
}
