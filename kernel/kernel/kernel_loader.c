/*Empty for now*/
#include <stdint.h>

#define VGA_ADDRESS 0xB8000
static const uint32_t VGA_WIDTH = 80;
static const uint32_t VGA_HEIGHT = 25;
volatile static char* vga = (volatile char*)VGA_ADDRESS;

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)
#define WHITE_ON_BLACK 0x0F

static int current_row = VGA_HEIGHT - 1;

void scroll_up()
{
    for (int row = 1; row < VGA_HEIGHT; ++row)
    {
        for (int col = 0; col < VGA_WIDTH; ++col)
        {
            VGA_MEMORY[(row - 1) * VGA_WIDTH + col] = VGA_MEMORY[row * VGA_WIDTH + col];
        }
    }

    // Clear last row
    for (int col = 0; col < VGA_WIDTH; ++col)
    {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = ' ' | (WHITE_ON_BLACK << 8);
    }
}

void debug_print(uint32_t value)
{
    scroll_up();

    int col = 0;
    for (int i = 7; i >= 0; --i)
    {
        char ch = (value >> (i * 4)) & 0xF;
        if (ch >= 10)
            ch += 'A' - 10;
        else
            ch += '0';

        VGA_MEMORY[current_row * VGA_WIDTH + col++] = ch | (WHITE_ON_BLACK << 8);
    }
}
void debug_print_int(int32_t value)
{
    scroll_up();

    int col = 0;
    char buffer[12]; // Enough for -2,147,483,648 and null terminator
    int idx = 0;

    if (value == 0)
    {
        buffer[idx++] = '0';
    }
    else
    {
        if (value < 0)
        {
            VGA_MEMORY[current_row * VGA_WIDTH + col++] = '-' | (WHITE_ON_BLACK << 8);
            value = -value;
        }

        int32_t tmp = value;
        while (tmp > 0)
        {
            buffer[idx++] = '0' + (tmp % 10);
            tmp /= 10;
        }
    }

    // Print in reverse
    while (--idx >= 0)
    {
        VGA_MEMORY[current_row * VGA_WIDTH + col++] = buffer[idx] | (WHITE_ON_BLACK << 8);
    }
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