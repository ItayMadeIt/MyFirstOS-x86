#ifndef __TTY_H__
#define __TTY_H__

#include "core/num_defs.h"

void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char* data, usize_ptr size);
void terminal_writestring(const char* data);


#endif // __TTY_H__