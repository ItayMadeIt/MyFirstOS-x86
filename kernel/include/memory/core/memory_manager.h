#ifndef __MEMORY_MANAGER_H__
#define __MEMORY_MANAGER_H__

#include "core/num_defs.h"
#include <memory/core/pfn_desc.h>
#include <core/defs.h>

enum mm_alloc_type 
{
    ALLOC_NONE = 0,
    ALLOC_BITMAP,
    ALLOC_FRAME,
};

void mm_ensure_memory(usize_ptr count);
void mm_reclaim_memory(usize_ptr count);

void mm_set_allocator_type(enum mm_alloc_type new_alloc_type);
void* mm_alloc_pagetable();

page_t* mm_alloc_pages(usize_ptr count);

void mm_get_page(page_t* desc);
void mm_put_page(page_t* desc);

void mm_get_range(page_t* begin, usize_ptr count);
void mm_put_range(page_t* begin, usize_ptr count);

#endif // __MEMORY_MANAGER_H__