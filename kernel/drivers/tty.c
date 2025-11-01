#include <stdbool.h>
#include <stddef.h>
#include "core/num_defs.h"
#include <string.h>

#include <drivers/tty.h>
#include <drivers/vga.h>

static const usize VGA_WIDTH = 80;
static const usize VGA_HEIGHT = 25;
static u16* const VGA_MEMORY = (u16*) 0xB8000;

static usize terminal_row;
static usize terminal_column;
static u8 terminal_color;
static u16* terminal_buffer;

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
	terminal_buffer = VGA_MEMORY;
	for (usize y = 0; y < VGA_HEIGHT; y++) 
	{
		for (usize x = 0; x < VGA_WIDTH; x++) 
		{
			const usize index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(u8 color) 
{
	terminal_color = color;
}

void terminal_putentryat(unsigned char c, u8 color, usize x, usize y) 
{
	const usize index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}
void terminal_scroll() 
{
	// Scroll all lines up by one
	for (usize row = 0; row < VGA_HEIGHT; row++) 
	{
		for (usize col = 0; col < VGA_WIDTH; col++) 
		{
			terminal_buffer[row * VGA_WIDTH + col] =
				terminal_buffer[(row + 1) * VGA_WIDTH + col];
		}
	}

	// Clear the last line
	for (usize col = 0; col < VGA_WIDTH; col++) 
	{
		terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + col] =
			vga_entry(' ', terminal_color);
	}
}


void terminal_nextline()
{
	terminal_column = 0;
	
	if (terminal_row + 1 != VGA_HEIGHT)
	{
		++terminal_row;
	}
	else
	{
		terminal_scroll();
	}
}

void terminal_putchar(char c) 
{
	if (c == '\n')
	{
		terminal_nextline();
		return;
	}

	unsigned char uc = c;
	terminal_putentryat(uc, terminal_color, terminal_column, terminal_row);
	if (terminal_column + 1 == VGA_WIDTH) 
	{
		terminal_nextline();
	}
	else
	{
		++terminal_column;
	}
}

void terminal_write(const char* data, usize_ptr size) 
{
	for (usize_ptr i = 0; i < size; i++)
	{
		terminal_putchar(data[i]);
	}
}

void terminal_writestring(const char* data) 
{
	terminal_write(data, strlen(data));
}
