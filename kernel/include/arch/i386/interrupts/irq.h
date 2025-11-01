#include "core/num_defs.h"

#include <kernel/interrupts/irq.h>

typedef struct __attribute__((packed)) irq_frame
{
    // pushed by stub
    u32 err_code;   // real or 0
    u32 irq_index;

    // pushad
    u32 edi;
    u32 esi;
    u32 ebp; 
    u32 esp_dummy;
    u32 ebx;
    u32 edx;
    u32 ecx;
    u32 eax;

    // CPU return state (normalized by stub if needed)
    u32 user_esp;    // 0 if not from ring3
    u32 user_ss;     // 0 if not from ring3
    u32 eflags;
    u32 cs;
    u32 eip;

} irq_frame_t;

void init_irq();