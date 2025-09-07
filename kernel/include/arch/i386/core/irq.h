#include <stdint.h>
#include <kernel/core/irq.h>

typedef struct __attribute__((packed)) irq_frame
{
    // pushed by stub
    uint32_t err_code;   // real or 0
    uint32_t irq_index;

    // pushad
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp; 
    uint32_t esp_dummy;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    // CPU return state (normalized by stub if needed)
    uint32_t user_esp;    // 0 if not from ring3
    uint32_t user_ss;     // 0 if not from ring3
    uint32_t eflags;
    uint32_t cs;
    uint32_t eip;

} irq_frame_t;

void set_idt_entry(uint32_t entry_index, void (*handler_addr), uint16_t selector, uint8_t type_attr);

void init_irq();