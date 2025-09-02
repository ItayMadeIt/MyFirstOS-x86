#include <multiboot/multiboot.h>
#include <core/debug.h>
#include <core/defs.h>
#include <early/defs.h>
#include <core/idt.h>
#include <core/gdt.h>

// constant offset: virt = phys + off
extern char __va_pa_off[];
extern char kernel_stack_top[];

void setup_gdt();
void setup_idt();
void setup_paging();
void setup_isr();

void kernel_main(multiboot_info_t* mbd);

EARLY_TEXT_SECTION
void entry_main(uint32_t magic, multiboot_info_t* mbd)
{
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        debug_print_str("Magic value wasn't present after boot.\n");
        halt();
    }

    // Critical setup 
    setup_gdt();
    setup_idt();

    setup_paging();

    setup_isr();

    // Call kernel_main with mbd
    uint32_t new_sp   = (uint32_t)kernel_stack_top;
    asm volatile(
        "movl %0, %%esp\n\t"      // switch to new (low-phys) stack
        "movl %0, %%ebp\n\t"
        "pushl %1\n\t"            // push mbd (already adjusted if needed)
        "call kernel_main\n\t"    // kernel_main(mbd)
        :
        : "r"(new_sp), "r"(mbd)
        : "memory", "cc"
    );
}