#include <string.h>

void* memset(void* bufptr, int value, usize size) {
	unsigned char* buf = (unsigned char*) bufptr;
	for (usize i = 0; i < size; i++)
		buf[i] = (unsigned char) value;
	return bufptr;
}
