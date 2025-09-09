#include <memory/core/memory_manager.h>
#include <memory/core/pfn_desc.h>
#include <memory/phys_alloc/bitmap_alloc.h>
#include <memory/phys_alloc/pfn_alloc.h>
#include <memory/heap/heap.h>
#include <kernel/boot/boot_data.h>

#define HEAP_VIRT   0xD0000000
#define KERNEL_ADDR 0x00100000

void* (*alloc_phys_page)(enum phys_page_type type, uint16_t flags);
void (*free_phys_page)(void* pa);

static void* dummy_alloc_phys_page(enum phys_page_type type, uint16_t flags) 
{
    (void)type;
    (void)flags;
    abort();
    return NULL;
}
static void dummy_free_phys_page(void* pa) 
{
    (void)pa;
    abort();
}


void init_memory(boot_data_t* boot_data)
{
    void* free_virt_addr = (void*)HEAP_VIRT;

    alloc_phys_page = dummy_alloc_phys_page;
    free_phys_page = dummy_free_phys_page;

    // init a sample physical allocator
    init_bitmap_phys_allocator(boot_data);
    alloc_phys_page = alloc_phys_page_bitmap;

    // Make a PFN based physical allocator
    init_pfn_descriptors(&free_virt_addr, boot_data);
    init_pfn_allocator(boot_data);
    alloc_phys_page = alloc_phys_page_pfn;
    free_phys_page = free_phys_page_pfn;

    // after setting up a proper alloc_phys_page we can now use `map_pages` for the heap

    uintptr_t heap_start = round_page_up(free_virt_addr);
    init_heap((void*)heap_start, STOR_2MiB);
}