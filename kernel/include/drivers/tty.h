#ifndef __TTY_H__
#define __TTY_H__


#include <stddef.h>

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);


#endif // __TTY_H__