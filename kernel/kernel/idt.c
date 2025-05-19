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


typedef struct idt_entry {
    uint16_t offset_low;            // Lower 16 bits of the handler function address
    uint16_t segment_selector;      // Segment selector (usually code segment)
    uint8_t  zero;                  // Must be zero
    uint8_t  type_attr;             // Type and attributes
    uint16_t offset_high;           // Upper 16 bits of the handler function address
} __attribute__((packed)) idt_entry_t;

idt_entry_t idt_entries[IDT_ENTRIES] __attribute__((aligned(16)));

typedef struct idt_descriptor 
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) idt_descriptor_t;

void idt_c_handler(uint8_t interrupt_number, uint32_t error_code)
{
    if (interrupt_number > 32)
    {
        debug_print_str("Interrupt number wasn't normal\n\n");
        debug_print(interrupt_number);

        // Sort of a halt
        while (1);

    }

    debug_print(interrupt_number);
}

static void set_idt_entry(idt_entry_t* entry, void (*handler_addr), uint16_t selector, uint8_t type_attr)
{
    entry->zero = 0;
    
    entry->offset_low = ((uint32_t)handler_addr) & 0xFFFF;
    entry->offset_high = ((uint32_t)handler_addr >> 16) & 0xFFFF;

    entry->segment_selector = selector;
    entry->type_attr = type_attr;
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

void setup_idt()
{
    extern void isr_stub_no_err();
    extern void isr_stub_err();

    for (uint16_t i = 0; i < IDT_ENTRIES; ++i) 
    {
        void (*callback) = isr_stub_no_err;
        uint8_t type_attr = IDT_INTERRUPT_32_DPL0; 

        switch (i) 
        {
            // IDT Trap entries:
            case 1:
            case 3:
            case 4:
                type_attr = IDT_TRAP_32_PL0;
                break;
            // IDT error entries:
            case 8:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 17:
            case 21:
                callback = isr_stub_err;
                break;
        }

        set_idt_entry(&idt_entries[i], callback, SEGMENT_SELECTOR_CODE_DPL0, type_attr);
    }

    idt_descriptor_t idt_descriptor;
    idt_descriptor.base = (uint32_t)&idt_entries;
    idt_descriptor.limit = sizeof(idt_entries)- 1;

    load_idt_descriptor(&idt_descriptor);
}
