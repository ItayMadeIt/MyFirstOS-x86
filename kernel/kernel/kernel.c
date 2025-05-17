#include <stdio.h>

#include <kernel/tty.h>
#include <stdint.h>

void kernel_main(void) 
{
	terminal_initialize();

#ifdef __is_libc
	printf("[ERROR] LIBC IS DEFINED/USED\n\n");	
#endif
    
    printf("\"Subscribe to the vultuous\" - Jonny Richardson, 2025\n\n\n\"We are proud of you!\" - Everyone, 2007-2025.");
}
