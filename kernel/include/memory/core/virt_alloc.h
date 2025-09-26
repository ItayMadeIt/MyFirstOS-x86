#ifndef __VIRT_ALLOC_H__
#define __VIRT_ALLOC_H__

#include <stdint.h>
#include <stdbool.h>

void* valloc_pages(uintptr_t count, uint32_t type, uint32_t page_flags);
void* vrealloc_pages(void* va_ptr, uintptr_t count, uint32_t new_flags);
void  vfree_pages(void* va_ptr);

#endif // __VIRT_ALLOC_H__