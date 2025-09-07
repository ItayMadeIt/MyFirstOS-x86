
#include <core/defs.h>
#include <arch/i386/core/idt.h>

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
