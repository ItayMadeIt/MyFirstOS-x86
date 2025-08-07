
#include <core/defs.h>

typedef struct idt_entry {
    uint16_t offset_low;            // Lower 16 bits of the handler function address
    uint16_t segment_selector;      // Segment selector (usually code segment)
    uint8_t  zero;                  // Must be zero
    uint8_t  type_attr;             // Type and attributes
    uint16_t offset_high;           // Upper 16 bits of the handler function address
} __attribute__((packed)) idt_entry_t;

typedef struct idt_descriptor 
{
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) idt_descriptor_t;

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
#define BASE_INTERRUPTS_ENTRIES 0x20

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

void set_interrupt_c_callback(uint8_t entry_index, void (*callback) (uint32_t error_code));
void set_idt_callback(uint16_t index, void (*handler_addr));
void set_idt_entry(uint32_t entry_index, void (*handler_addr), uint16_t selector, uint8_t type_attr);