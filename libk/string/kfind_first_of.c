#include "string.h"

iptr kfind_index_first_of_from(const char* s1, const char target, usize_ptr offset)
{
    const char* it = s1 + offset;
    while (*it != target)
    {
        if (*it == 0)
        {
            return -1;
        }

        it++;
    }
    return it - s1;
}