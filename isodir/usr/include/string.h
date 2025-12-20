#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int memcmp(const void*, const void*, usize_ptr);
void* memcpy(void* __restrict, const void* __restrict, usize_ptr);
void* memmove(void*, const void*, usize_ptr);
void* memset(void*, int, usize_ptr);
usize_ptr strlen(const char*);

#ifdef __cplusplus
}
#endif

#endif
