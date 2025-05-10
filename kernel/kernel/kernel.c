#include <stdio.h>

#include <kernel/tty.h>
#include <stdint.h>

static void debug_print(uint32_t value)
{
    volatile static char* vga = (volatile char*)0xB8000;
    vga+=sizeof(uint32_t) * 4 - 1;

    uint8_t it = 8;

    while (it)
    {
        char ch = (value & 0xF);
        if (ch >= 10)
        {
            ch += 'A' - 10;
        }
        else 
        {
            ch += '0';
        }

        *vga-- = 0x0F;
        *vga-- = ch;

        value >>= 4;
        --it;
    }
    vga += 80 * 2 + 1;
}
void kernel_main(void) 
{
    volatile char* test = (volatile char*)0xC0000000;
    *test = 'K';
    *(test + 1) = 0x0F;

    debug_print(*test);
	
	return 0;

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
