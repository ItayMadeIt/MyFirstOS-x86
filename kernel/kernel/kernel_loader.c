/*Empty for now*/
#include <stdint.h>

#define VGA_ADDRESS 0xB8000
static const uint32_t VGA_WIDTH = 80;
static const uint32_t VGA_HEIGHT = 25;
volatile static char* vga = (volatile char*)VGA_ADDRESS;

void debug_print(uint32_t value)
{
    for (int i = 7; i >= 0; --i)
    {
        char ch = (value >> (i * 4)) & 0xF;
        if (ch >= 10)
            ch += 'A' - 10;
        else
            ch += '0';

        *vga++ = ch;
        *vga++ = 0x0F;
    }
    debug_print_str("\n");
}

void debug_print_str(const char* str) 
{
    while (*str)
    {
        if (*str == '\n')
        {
            uint32_t offset = (vga - (volatile char*)VGA_ADDRESS) / 2;
            uint32_t next_line_offset = ((offset / VGA_WIDTH) + 1) * VGA_WIDTH;
            
            vga = (volatile char*)VGA_ADDRESS + next_line_offset * 2;
            ++str;

            continue;   
        }
        
        *vga++ = *str++;
        *vga++ = 0x0F;
    }
}