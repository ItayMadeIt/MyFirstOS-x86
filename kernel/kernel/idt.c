#include <stdint.h>
#include <stdbool.h>

// Descriptor types for the IDT

#define IDT_PRESENT(x)           ((x) << 7)           // Present bit (must be 1 for active entries)
#define IDT_DPL(x)               (((x) & 0b11) << 5)  // Descriptor Privilege Level (0-3)
#define IDT_SELECTOR(x)          ((x) & 0xFFFF)       // Lower 16 bits for segment selector

#define IDT_FLAGS(type, dpl)  (IDT_PRESENT(1) | IDT_DPL(dpl) | (type & 0xF))

#define IDT_TYPE_INTERRUPT_GATE  0xE   // 32-bit interrupt gate
#define IDT_TYPE_TRAP_GATE       0xF   // 32-bit trap gate
#define IDT_TYPE_TASK_GATE       0x5   // Task gate


#define IDT_INTERRUPT_32_DPL0   IDT_FLAGS(IDT_TYPE_INTERRUPT_GATE, 0)
#define IDT_TRAP_32_PL0        IDT_FLAGS(IDT_TYPE_TRAP_GATE, 0)

#define SEGMENT_SELECTOR_CODE_DPL0 IDT_SELECTOR(0x08)

#define IDT_ENTRIES 256

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

typedef struct idt_entry {
    uint16_t offset_low;            // Lower 16 bits of the handler function address
    uint16_t segment_selector;      // Segment selector (usually code segment)
    uint8_t  zero;                  // Must be zero
    uint8_t  type_attr;             // Type and attributes
    uint16_t offset_high;           // Upper 16 bits of the handler function address
} __attribute__((packed)) idt_entry_t;

idt_entry_t idt_entries[IDT_ENTRIES] __attribute__((aligned(16)));
void (*interrupt_callback_entries[IDT_ENTRIES]) (uint32_t error_code);

typedef struct idt_descriptor 
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) idt_descriptor_t;

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

void set_interrupt_callback(uint8_t entry_index, void (*callback) (uint32_t error_code))
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
