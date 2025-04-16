#include <stdio.h>

#include <kernel/tty.h>

void kernel_main(void) 
{
	terminal_initialize();

#ifdef __is_libc
	printf("[ERROR] LIBC IS DEFINED/USED\n\n");	
#endif


	printf("Good day %s\n\n", "Prince");
	printf("Good day %s\n\n", "Princess");
	printf("Good day %s\n\n", "Princesses");

	printf("Good day %s\n\n", "Prince1");
	printf("Good day %s\n\n", "Princess1");
	printf("Good day %s\n\n", "Princesses1");

	printf("Good day %s\n\n", "Prince2");
	printf("Good day %s\n\n", "Princess2");
	printf("Good day %s\n\n", "Princesses2");

}
