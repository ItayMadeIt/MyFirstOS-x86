#ifndef __BITMAP_ALLOC_H__
#define __BITMAP_ALLOC_H__

#include <memory/phys_alloc/phys_alloc.h>
#include <core/defs.h>
#include <kernel/boot/boot_data.h>
#include <stdint.h>

// Max memory space for bitmap physical allocator 4GB:
#define MAX_BASE_MEM_SPACE (STOR_2GiB)
#define PAGES_ARR_SIZE (MAX_BASE_MEM_SPACE / PAGE_SIZE / BIT_TO_BYTE)

#define MAX_MEMORY_ENTRIES 256

void* alloc_phys_page_bitmap ();
phys_alloc_t alloc_phys_pages_bitmap(uintptr_t count);

void free_phys_page_bitmap(void* page_addr);
void free_phys_pages_bitmap(phys_alloc_t free_params);

void init_bitmap_phys_allocator(boot_data_t* boot_data);
#endif // __BITMAP_ALLOC_H__