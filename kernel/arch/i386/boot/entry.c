#include <arch/i386/boot/early.h>
#include <early/defs.h>
#include "core/num_defs.h"
#include <kernel/core/cpu.h>
#include <arch/i386/early/irq.h>
#include <arch/i386/boot/entry_data.h>

void kernel_main(boot_data_t* data);

static boot_data_t entry_data;
void jump_high_kernel(usize_ptr stack_top, boot_data_t data)
{
    entry_data = data;

    asm volatile(
        "movl %0, %%esp\n\t"      // switch to new stack
        "movl %0, %%ebp\n\t"
        :
        : "r"(stack_top)
        : "memory", "cc"
    );

    kernel_main(&entry_data);
}


// Entry main acts as a trampoline
EARLY_TEXT_SECTION
void entry_main(u32 magic, multiboot_info_t* mbd)
{
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        cpu_halt();
    }

    // Critical setup 
    early_irq_disable();
    init_gdt();
    init_boot_isr();
    init_paging();

    // Call kernel_main with mbd
    boot_data_t data = {
        .mbd = mbd
    };

    u32 new_stack_top = (u32)kernel_stack_top;
    jump_high_kernel(new_stack_top, data);
}