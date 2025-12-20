#ifndef __BITMAP_ALLOC_H__
#define __BITMAP_ALLOC_H__

#include <core/defs.h>
#include <kernel/boot/boot_data.h>
#include "core/num_defs.h"


// Max memory space for bitmap physical allocator 4GB:
#define MAX_BASE_MEM_SPACE (STOR_2GiB)
#define PAGES_ARR_SIZE (MAX_BASE_MEM_SPACE / PAGE_SIZE / BIT_TO_BYTE)

#define MAX_MEMORY_ENTRIES 256

typedef struct phys_alloc
{
    void* addr;
    usize_ptr count;
} phys_alloc_t;

void* bitmap_alloc_page ();
phys_alloc_t bitmap_alloc_pages(usize_ptr count);

void free_phys_page_bitmap(void* page_addr);
void free_phys_pages_bitmap(phys_alloc_t free_params);

void init_bitmap_phys_allocator(boot_data_t* boot_data);
#endif // __BITMAP_ALLOC_H__