#include <stdint.h>
#include <drivers/tty.h>
#include <boot/acpi.h>
#include <stdio.h>
#include <multiboot/multiboot.h>

void setup_memory(multiboot_info_t* mbd);
void setup_isr();
void setup_pic();
void setup_pit();
void setup_ps2();

void halt() 
{
    asm volatile (
        "cli\n\t"   // disable interrupts
        "hlt\n\t"   // halt the CPU
    );
}
void abort()
{
    // for now
    halt();
}
void assert(bool must_be_true)
{
    if (! must_be_true) 
    {   
		printf("Aborted\n");
        halt();
    }
}

void kernel_main(multiboot_info_t* mbd)
{
    // Setup interrupt handlers
	setup_memory(mbd);

    // inputs using pic1, pic2 (slave)
    setup_pic();

    // basic drivers
    setup_pit();
    setup_ps2();

    setup_acpi();

	sti();

	terminal_initialize();

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
