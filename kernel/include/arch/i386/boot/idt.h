#include <core/defs.h>
#include <arch/i386/core/idt.h>

#define IDT_ENTRIES 256

void early_set_interrupt_c_callback(u8 entry_index, void (*callback) (u32 error_code));

void early_isr0();
void early_isr1();
void early_isr2();
void early_isr3();
void early_isr4();
void early_isr5();
void early_isr6();
void early_isr7();
void early_isr8();
void early_isr9();
void early_isr10();
void early_isr11();
void early_isr12();
void early_isr13();
void early_isr14();
void early_isr15();
void early_isr16();
void early_isr17();
void early_isr18();
void early_isr19();
void early_isr20();
void early_isr21();
void early_isr22();
void early_isr23();
void early_isr24();
void early_isr25();
void early_isr26();
void early_isr27();
void early_isr28();
void early_isr29();
void early_isr30();
void early_isr31();
