#include <core/debug.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define WHITE_ON_BLACK 0x0F
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

static int cursor_row = 0;
static int cursor_col = 0;

void scroll_up() {
    for (int row = 1; row < VGA_HEIGHT; ++row) 
    {
        for (int col = 0; col < VGA_WIDTH; ++col) 
        {
            VGA_MEMORY[(row - 1) * VGA_WIDTH + col] = VGA_MEMORY[row * VGA_WIDTH + col];
        }
    }

    for (int col = 0; col < VGA_WIDTH; ++col) {
        VGA_MEMORY[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = ' ' | (WHITE_ON_BLACK << 8);
    }

    if (cursor_row > 0) cursor_row--;
}

void put_char(char ch) {
    if (ch == '\n') {
        cursor_row++;
        cursor_col = 0;
    } else {
        VGA_MEMORY[cursor_row * VGA_WIDTH + cursor_col] = ch | (WHITE_ON_BLACK << 8);
        cursor_col++;
        if (cursor_col >= VGA_WIDTH) {
            cursor_col = 0;
            cursor_row++;
        }
    }

    if (cursor_row >= VGA_HEIGHT) {
        scroll_up();
    }
}

void debug_print_str(const char* str) {
    while (*str) {
        put_char(*str++);
    }
}

void debug_print(uint64_t value) 
{
    char hex[] = "0123456789ABCDEF";
    for (int i = sizeof(value)*2-1; i >= 0; --i) 
    {
        char ch = hex[(value >> (i * 4)) & 0xF];
        put_char(ch);
    }
    put_char('\n');
}

void debug_print_int_nonewline(int64_t value) 
{
    char buffer[12];
    int i = 0;

    if (value < 0) 
    {
        put_char('-');
        value = -value;
    }

    if (value == 0) 
    {
        put_char('0');
        return;
    }

    while (value > 0) 
    {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i--) 
    {
        put_char(buffer[i]);
    }
}

void debug_print_int(int64_t value) 
{
    debug_print_int_nonewline(value);
    put_char('\n');
}