#include <stdio.h>
#include <core/defs.h>
#include <drivers/tty.h>
#include <firmware/acpi.h>
#include <multiboot/multiboot.h>

#include <kernel/drivers/keyboard.h>
#include <kernel/drivers/int_timer.h>
#include <kernel/core/irq.h>
#include <kernel/core/cpu.h>

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
static bool dummy_printing_time;

static void dummy_key_handler(raw_key_event_t key_event)
{
    if (key_event.pressed == false)
    {
        return;
    }    

    switch (key_event.code) {
        // digits
        case KEYCODE_0: printf("0"); break;
        case KEYCODE_1: printf("1"); break;
        case KEYCODE_2: printf("2"); break;
        case KEYCODE_3: printf("3"); break;
        case KEYCODE_4: printf("4"); break;
        case KEYCODE_5: printf("5"); break;
        case KEYCODE_6: printf("6"); break;
        case KEYCODE_7: printf("7"); break;
        case KEYCODE_8: printf("8"); break;
        case KEYCODE_9: printf("9"); break;

        // letters
        case KEYCODE_A: printf("a"); break;
        case KEYCODE_B: printf("b"); break;
        case KEYCODE_C: printf("c"); break;
        case KEYCODE_D: printf("d"); break;
        case KEYCODE_E: printf("e"); break;
        case KEYCODE_F: printf("f"); break;
        case KEYCODE_G: printf("g"); break;
        case KEYCODE_H: printf("h"); break;
        case KEYCODE_I: printf("i"); break;
        case KEYCODE_J: printf("j"); break;
        case KEYCODE_K: printf("k"); break;
        case KEYCODE_L: printf("l"); break;
        case KEYCODE_M: printf("m"); break;
        case KEYCODE_N: printf("n"); break;
        case KEYCODE_O: printf("o"); break;
        case KEYCODE_P: printf("p"); break;
        case KEYCODE_Q: printf("q"); break;
        case KEYCODE_R: printf("r"); break;
        case KEYCODE_S: printf("s"); break;
        case KEYCODE_T: printf("t"); break;
        case KEYCODE_U: printf("u"); break;
        case KEYCODE_V: printf("v"); break;
        case KEYCODE_W: printf("w"); break;
        case KEYCODE_X: printf("x"); break;
        case KEYCODE_Y: printf("y"); break;
        case KEYCODE_Z: printf("z"); break;
        case KEYCODE_ENTER: printf("\n"); break;
        case KEYCODE_SPACE: printf(" "); break;
        case KEYCODE_MINUS: dummy_printing_time = !dummy_printing_time; break;

        default:
            break;
    }
}

static void dummy_time_event(int_timer_event_t time_event)
{
    if (dummy_printing_time == false)
        return;
    printf(
        "Tick #%llu Time:%llu [hz:%d]\n", 
        time_event.tick_count, 
        time_event.timestamp_ns,
        time_event.frequency_hz
    );
}

void kernel_main(multiboot_info_t* mbd)
{
    cpu_init();

	terminal_initialize();

    // Setup interrupt handlers
	setup_memory(mbd);

    // basic drivers
    init_int_timer(10, dummy_time_event);
    init_keyboard(dummy_key_handler);

    // setup_acpi();

	irq_enable();

#ifdef __is_libc
	printf("[ERROR] LIBC IS DEFINED/USED\n\n");	
#endif

	while(1)
    {
        //printf("Hi!!!\n\n");	
    }
}