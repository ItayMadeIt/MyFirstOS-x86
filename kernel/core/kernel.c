#include <stdio.h>
#include <core/defs.h>
#include <drivers/tty.h>
#include <arch/int_timer.h>
#include <arch/keyboard.h>
#include <boot/acpi.h>
#include <multiboot/multiboot.h>
#include <arch/irq.h>
#include <arch/cpu.h>

void setup_memory(multiboot_info_t* mbd);
void init_boot_isr();
void setup_pic();
void setup_pit();
void setup_ps2();

void abort()
{
    // for now
    cpu_halt();
}

void assert(bool must_be_true)
{
    if (! must_be_true) 
    {   
		printf("Aborted\n");
        abort();
    }
}
static void dummy()
{

}

void kernel_main(multiboot_info_t* mbd)
{
	terminal_initialize();

    // Setup interrupt handlers
	setup_memory(mbd);

    // inputs using pic1, pic2 (slave)
    setup_pic();

    // basic drivers
    init_int_timer(20);
    init_keyboard(dummy);

    setup_acpi();

	//irq_enable();

	printf("Hi!!!\n\n");	

#ifdef __is_libc
	printf("[ERROR] LIBC IS DEFINED/USED\n\n");	
#endif

	while(1)
    {
        //printf("Hi!!!\n\n");	
    }
}
// fix linker errors likee linker does stupid shit because it's not really linked well
