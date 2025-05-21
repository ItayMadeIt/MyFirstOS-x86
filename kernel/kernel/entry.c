#include <stdint.h>
#include <stdbool.h>
#include <kernel_loader/io.h>

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

void halt() 
{
    asm volatile (
        "cli\n\t"   // disable interrupts
        "hlt\n\t"   // halt the CPU
    );
}


void virt_kernel_main()
{
    asm volatile (
        "mov $kernel_main, %%eax\n\t"
        "add $0xBFF00000, %%eax\n\t"
        "call *%%eax\n\t"
        : 
        : 
        : "eax", "memory"
    );
}

void IRQ0_handler();
typedef struct _ idt_entry_t;
void set_idt_entry(idt_entry_t* entry, void (*handler_addr), uint16_t selector, uint8_t type_attr);

void setup_pit()
{
    set_idt_entry(0x20, IRQ0_handler, SEGMENT_SELECTOR_CODE_DPL0, IDT_INTERRUPT_32_DPL0);

    const uint32_t frequency = 1000; // 1000 Hz

    asm volatile (
        "mov %0, %%ebx\n\r"
        "call setup_pit_asm\n\r"
        :
        : "r" (frequency)
        : "memory"
    );
}

static inline void sti()
{
    asm volatile("sti");
}

static inline int interrupts_enabled(void) 
{
    unsigned long eflags;

    asm volatile (
        "pushf\n\t"
        "pop %0"
        : "=r"(eflags)
    );
    
    return (eflags & (1 << 9)) ? 1 : 0;
}



void entry_main()
{
    // Critical setup
    setup_gdt();
    setup_idt();
    setup_paging();
    
    // Setup interrutp handlers
    setup_isr();

    // Setup inputs using pic1, pic2
    setup_pic();
    
    // Setup basic one-core timer
    setup_pit();

    sti();

    while(1);

    //virt_kernel_main();
}