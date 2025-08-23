#ifndef __PHYS_ALLOC_H__
#define __PHYS_ALLOC_H__

#include <multiboot/multiboot.h>

// Max memory space 4GB:
#define MAX_MEM_SPACE 4294967296
#define PAGES_ARR_SIZE MAX_MEM_SPACE / PAGE_SIZE / BIT_TO_BYTE

#define MAX_MEMORY_ENTRIES 256

typedef struct phys_memory_list
{
    multiboot_memory_map_t* mmmt[MAX_MEMORY_ENTRIES];
    uint16_t amount;
} phys_memory_list_t;

extern uint64_t max_memory;

extern uint8_t linker_kernel_begin;
extern uint8_t linker_kernel_end;

extern uint32_t kernel_begin_pa;
extern uint32_t kernel_end_pa;

void* alloc_phys_page_bitmap(uint32_t page_type, uint32_t page_flags);
void free_phys_page_bitmap(uint32_t page_addr);

void setup_phys_allocator(multiboot_info_t* mbd);
#endif // __PHYS_ALLOC_H__