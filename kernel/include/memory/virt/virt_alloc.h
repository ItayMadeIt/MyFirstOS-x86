#ifndef __VIRT_ALLOC_H__
#define __VIRT_ALLOC_H__

#include "core/num_defs.h"
#include "memory/virt/virt_region.h"

void* kvalloc_pages(
    usize_ptr count, 
    enum virt_region_type vregion
);

void  kvfree_pages(void* va_ptr);

#endif // __VIRT_ALLOC_H__