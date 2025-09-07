#include <arch/i386/early/early.h>
#include <early/defs.h>
#include <kernel/core/cpu.h>
#include <kernel/core/irq.h>
#include <multiboot/multiboot.h>

void kernel_main(multiboot_info_t* mbd);

void jump_high_kernel(uint32_t stack_top, multiboot_info_t* mbd)
{
    asm volatile(
        "movl %0, %%esp\n\t"      // switch to new (low-phys) stack
        "movl %0, %%ebp\n\t"
        "pushl %1\n\t"            // push mbd (already adjusted if needed)
        "call kernel_main\n\t"    // kernel_main(mbd)
        :
        : "r"(stack_top), "r"(mbd)
        : "memory", "cc"
    );
}

// constant offset: virt = phys + off
extern char __va_pa_off[];
extern char kernel_stack_top[];

// Entry main acts as a trampoline
EARLY_TEXT_SECTION
void entry_main(uint32_t magic, multiboot_info_t* mbd)
{
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        cpu_halt();
    }

    // Critical setup 
    init_gdt();
    init_boot_isr();

    init_paging();

    irq_disable();

    // Call kernel_main with mbd
    uint32_t new_stack_top = (uint32_t)kernel_stack_top;
    jump_high_kernel(new_stack_top, mbd);
}