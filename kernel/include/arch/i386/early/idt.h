
#include <core/defs.h>

typedef struct idt_entry {
    uint16_t offset_low;            // Lower 16 bits of the handler function address
    uint16_t segment_selector;      // Segment selector (usually code segment)
    uint8_t  zero;                  // Must be zero
    uint8_t  type_attr;             // Type and attributes
    uint16_t offset_high;           // Upper 16 bits of the handler function address
} __attribute__((packed, aligned(16))) idt_entry_t;

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

extern void early_isr0();
extern void early_isr1();
extern void early_isr2();
extern void early_isr3();
extern void early_isr4();
extern void early_isr5();
extern void early_isr6();
extern void early_isr7();
extern void early_isr8();
extern void early_isr9();
extern void early_isr10();
extern void early_isr11();
extern void early_isr12();
extern void early_isr13();
extern void early_isr14();
extern void early_isr15();
extern void early_isr16();
extern void early_isr17();
extern void early_isr18();
extern void early_isr19();
extern void early_isr20();
extern void early_isr21();
extern void early_isr22();
extern void early_isr23();
extern void early_isr24();
extern void early_isr25();
extern void early_isr26();
extern void early_isr27();
extern void early_isr28();
extern void early_isr29();
extern void early_isr30();
extern void early_isr31();

void early_set_interrupt_c_callback(uint8_t entry_index, void (*callback) (uint32_t error_code));
void early_set_idt_callback(uint16_t index, void (*handler_addr));
void early_set_idt_entry(uint32_t entry_index, void (*handler_addr), uint16_t selector, uint8_t type_attr);
