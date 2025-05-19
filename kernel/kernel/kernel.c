#include <stdio.h>

#include <kernel/tty.h>
#include <stdint.h>

void kernel_main(void) 
{
	terminal_initialize();

#ifdef __is_libc
	printf("[ERROR] LIBC IS DEFINED/USED\n\n");	
#endif

	printf("\'Execute order 66\' - Yotam Dekel Tzelgov, 2025\n\n\'Proud of you!\' - Everyone, all the time..");
}
