#include <kernel/core/irq.h>
#include <kernel/core/cpu.h>
#include <arch/i386/core/idt.h>
#include <arch/i386/core/irq.h>

idt_entry_t idt_entries[IDT_ENTRIES];
void (*interrupt_callback_entries[IDT_ENTRIES]) (irq_frame_t* data);

void set_idt_entry(uint32_t entry_index, void (*handler_addr_ptr), uint16_t selector, uint8_t type_attr)
{
    idt_entries[entry_index].zero = 0;
    
    uintptr_t handler_addr = (uintptr_t)handler_addr_ptr;
    idt_entries[entry_index].offset_low = ((uint32_t)handler_addr) & 0xFFFF;
    idt_entries[entry_index].offset_high = ((uint32_t)handler_addr >> 16) & 0xFFFF;

    idt_entries[entry_index].segment_selector = selector;
    idt_entries[entry_index].type_attr = type_attr;
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

void irq_enable()
{
    asm volatile (
        "sti\n\t"
    );
}

void irq_disable()
{
    asm volatile (
        "cli\n\t"
    );
}

void idt_c_handler(irq_frame_t* frame)
{
    uint32_t v = frame->irq_index;

    void (*callback)(irq_frame_t*) = interrupt_callback_entries[v];
    
    if (callback) 
    {
        callback(frame);
    }
}

uintptr_t irq_save()
{
    uintptr_t flags;
    asm volatile(
        "pushf\n\t"
        "pop %0\n\t"
        "cli\n\t"
        : "=r"(flags)
        : 
        : "memory"
    );

    return flags;
}

void irq_restore(uintptr_t flags)
{
    asm volatile(
        "push %0\n\t"
        "popf \n\t"
        : 
        : "r"(flags)
        : "memory"
    );
}

void irq_register_handler(uint32_t vector, void (*handle)(irq_frame_t*))
{
    interrupt_callback_entries[vector] = handle;
}


void init_irq()
{
    for (uint16_t i = 0; i < IDT_ENTRIES; ++i) 
    {
        interrupt_callback_entries[i] = 0;
    
        set_idt_entry(
            i,
            cpu_halt,
            SEGMENT_SELECTOR_CODE_DPL0, 
            IDT_INTERRUPT_32_DPL0
        );
    }

    // Normal interrupts
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

    // IRQs
    set_idt_entry(0x20, isr32, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x21, isr33, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x22, isr34, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x23, isr35, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x24, isr36, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x25, isr37, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x26, isr38, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x27, isr39, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x28, isr40, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x29, isr41, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x2A, isr42, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x2B, isr43, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x2C, isr44, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x2D, isr45, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x2E, isr46, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);
    set_idt_entry(0x2F, isr47, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);


    idt_descriptor_t idt_descriptor;
    idt_descriptor.base = (uint32_t)&idt_entries;
    idt_descriptor.limit = sizeof(idt_entries)- 1;

    load_idt_descriptor(&idt_descriptor);
}