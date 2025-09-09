#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdint.h>

void debug_print_str(const char* str);
void debug_print(uint64_t value) ;
void debug_print_int_nonewline(int64_t value);
void debug_print_int(int64_t value) ;

#endif // __DEBUG_H__