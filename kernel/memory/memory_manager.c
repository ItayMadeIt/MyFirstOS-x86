#include <core/paging.h>
#include <memory/memory_manager.h>
#include <memory/phys_alloc.h>
#include <memory/page_frame.h>
#include <memory/heap.h>

#define HEAP_VIRT   0xD0000000
#define KERNEL_ADDR 0x00100000

void* (*alloc_phys_page)(enum phys_page_type type, uint32_t flags);
bool (*map_page )(void* pa, void* va, uint16_t flags, enum phys_page_type type);

void setup_memory(multiboot_info_t* mbd)
{
    setup_phys_allocator(mbd);
    alloc_phys_page = alloc_phys_page_bitmap;

    uint32_t page_desc_end_mem = setup_page_descriptors(HEAP_VIRT, mbd);
    alloc_phys_page = alloc_phys_page_pfn;
    // after setting up a proper alloc_phys_page we can now use `map_pages` for the heap
    
    uint32_t heap_start = round_page_up(page_desc_end_mem);
    setup_heap((void*)heap_start, STOR_2MiB);
}