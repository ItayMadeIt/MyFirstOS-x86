#include <stddef.h>
#include "core/num_defs.h"

int strncmp(const char *s1, const char *s2, usize_ptr n) 
{
    usize_ptr i = 0;

    while (i < n && s1[i] != '\0' && s2[i] != '\0') 
    {
        if (s1[i] != s2[i]) 
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        i++;
    }

    if (i < n) 
        return (unsigned char)s1[i] - (unsigned char)s2[i];

    return 0;
}
