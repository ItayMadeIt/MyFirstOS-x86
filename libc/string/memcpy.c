#include <string.h>

void* memcpy(void* restrict dstptr, const void* restrict srcptr, usize size) {
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	for (usize i = 0; i < size; i++)
		dst[i] = src[i];
	return dstptr;
}
