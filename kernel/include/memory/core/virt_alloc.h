#ifndef __VIRT_ALLOC_H__
#define __VIRT_ALLOC_H__

#include <stdint.h>
#include <stdbool.h>
#include <kernel/memory/paging.h>

void* kvalloc_pages(uintptr_t count, uint16_t page_type, uint16_t page_flags);
void  kvfree_pages(void* va_ptr);

// need to do it: void* vmap(uintptr_t* phys_pages, uint64_t page_count, uint16_t flags);
// need to do it: void  vunmap(void* addr, uint64_t page_count);

void init_virt_alloc();
void virt_mark_region(void* from, void* to, const char* name);

// (currently not implemented) 
// void* kvrealloc_pages(void* va_ptr, uintptr_t count, uint16_t page_type, uint16_t page_flags);

#endif // __VIRT_ALLOC_H__