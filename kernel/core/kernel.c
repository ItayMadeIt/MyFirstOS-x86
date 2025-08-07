#include <stdio.h>
#include <stdint.h>
#include <drivers/tty.h>

void kernel_main(void)
{
	terminal_initialize();

#ifdef __is_libc
	printf("[ERROR] LIBC IS DEFINED/USED\n\n");	
#endif

	printf("Gunny is amazing!\n");

	while(1);
}
