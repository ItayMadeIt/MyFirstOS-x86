#ifndef _STRING_H
#define _STRING_H 1

#include <sys/cdefs.h>

int memcmp(const void*, const void*, usize);
void* memcpy(void* __restrict, const void* __restrict, usize);
void* memmove(void*, const void*, usize);
void* memset(void*, int, usize);
usize strlen(const char*);
int strncmp(const char* s1, const char* s2, usize n);
int strcmp(const char* s1, const char* s2);

#endif
