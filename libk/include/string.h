#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>
#include <core/num_defs.h>

i32 memcmp(const void*, const void*, usize_ptr);
void* memcpy(void* __restrict, const void* __restrict, usize_ptr);
void* memmove(void* dstptr, const void* srcptr, usize_ptr size);
void* memset(void*, int, usize_ptr);
usize_ptr strlen(const char*);
int strncmp(const char* s1, const char* s2, usize_ptr n);
int strcmp(const char* s1, const char* s2);
char* kstrdup(const char* s1);
iptr kfind_index_first_of_from(const char* s1, const char target, usize_ptr offset);

#endif
