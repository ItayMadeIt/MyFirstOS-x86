#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>
#include <core/num_defs.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

i32 memcmp(const void*, const void*, usize_ptr);
void* memcpy(void* __restrict, const void* __restrict, usize_ptr);
void* memmove(void*, const void*, usize_ptr);
void* memset(void*, i32, usize_ptr);
usize_ptr strlen(const char*);

#ifdef __cplusplus
}
#endif

#endif
