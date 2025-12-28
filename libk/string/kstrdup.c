#include "memory/heap/heap.h"
#include "sys/num_defs.h"
#include <string.h>

char* kstrdup(const char* s1)
{
    usize_ptr length = strlen(s1);

    char* new_str = kmalloc(strlen(s1) + 1);
    memcpy(new_str, s1, length);

    new_str[length] = '\0';

    return new_str;
}