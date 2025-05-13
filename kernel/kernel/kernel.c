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
	terminal_initialize();

#ifdef __is_libc
	printf("[ERROR] LIBC IS DEFINED/USED\n\n");	
#endif
    
    printf("\"I'm always correct.\" - Gunny Sela, 2025\n\"Proud of you\" - everyone, 2025");
}
