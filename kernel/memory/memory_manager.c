#include <memory/memory_manager.h>
#include <memory/heap.h>
#include <memory/phys_alloc.h>
#include <memory/page_frame.h>

#define HEAP_VIRT   0xD0000000
#define KERNEL_ADDR 0x00100000

void* (*alloc_phys_page)(enum phys_page_type type);

void setup_memory(multiboot_info_t* mbd)
{
    setup_phys_allocator(mbd);
    alloc_phys_page = alloc_phys_page_bitmap;

    uint32_t heap_begin = setup_page_descriptors(HEAP_VIRT, mbd);
    alloc_phys_page = alloc_phys_page_pfn;

    setup_heap(heap_begin);

    //setup_vma();
}