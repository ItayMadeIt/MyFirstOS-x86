#ifndef __PHYS_ALLOC_H__
#define __PHYS_ALLOC_H__

#include <core/defs.h>
#include <kernel/boot/boot_data.h>
#include <stdint.h>

// Max memory space for bitmap physical allocator 4GB:
#define MAX_BASE_MEM_SPACE (STOR_2GiB)
#define PAGES_ARR_SIZE (MAX_BASE_MEM_SPACE / PAGE_SIZE / BIT_TO_BYTE)

#define MAX_MEMORY_ENTRIES 256

extern uintptr_t max_memory;

enum phys_page_type;

void* alloc_phys_page_bitmap(enum phys_page_type page_type, uint16_t page_flags);
void free_phys_page_bitmap(void* page_addr);

void init_bitmap_phys_allocator(boot_data_t* boot_data);
#endif // __PHYS_ALLOC_H__