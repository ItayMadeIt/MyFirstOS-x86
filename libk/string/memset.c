#include "core/num_defs.h"
#include <string.h>

void* memset(void* bufptr, int value, usize_ptr size) {
	unsigned char* buf = (unsigned char*) bufptr;
	for (usize_ptr i = 0; i < size; i++)
		buf[i] = (unsigned char) value;
	return bufptr;
}
