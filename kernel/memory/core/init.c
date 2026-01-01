
#include "memory/core/init.h"
#include "core/defs.h"
#include "memory/core/memory_manager.h"
#include "memory/heap/heap.h"
#include "memory/phys_alloc/bitmap_alloc.h"
#include "memory/virt/virt_region.h"
#include "memory/core/linker_vars.h"
#include <stdio.h>

#define MM_AREA_VIRT   0xD0000000
#define KERNEL_ADDR    0x00100000
#define KERNEL_VIRT_ADDR 0xC0000000
#define HEAP_MAX_SIZE  STOR_256MiB

usize_ptr max_memory;

usize_ptr kernel_begin_pa;
usize_ptr kernel_end_pa;
usize_ptr kernel_size;


typedef struct interval
{
    usize_ptr begin;
    usize_ptr end;
} interval_t;

static void verify_flags(const boot_data_t* boot_data)
{
    if (!boot_has_memory(boot_data))
    {
        printf("Memory flag isn't enable.\n");
        abort();
    }
}

void init_memory(boot_data_t* boot_data)
{
    // PAGING CHANGED FROM PAGING FLALGS TO HW FLAGS
    kernel_begin_pa = (usize_ptr)&linker_kernel_begin;
    kernel_end_pa = (usize_ptr)&linker_kernel_end;

    verify_flags(boot_data);
    max_memory = get_max_memory(boot_data);

    mm_set_allocator_type(ALLOC_NONE);
    
    // init a sample physical allocator
    init_bitmap_phys_allocator(boot_data);
    mm_set_allocator_type(ALLOC_BITMAP);
    
    void* free_virt_addr = (void*)MM_AREA_VIRT;

    // Make a PFN based physical allocator
    interval_t pfn_interval;
    pfn_interval.begin = (usize_ptr) free_virt_addr;
    init_pfn_descriptors(&free_virt_addr, boot_data);
    pfn_interval.end = (usize_ptr) free_virt_addr;

    mm_set_allocator_type(ALLOC_FRAME);

    usize_ptr heap_begin = round_page_up(free_virt_addr);
    usize_ptr heap_init_size = clamp(max_memory / 16, STOR_8MiB, STOR_128MiB);
    usize_ptr heap_max_size = clamp(max_memory/4, STOR_32MiB, STOR_256MiB);
    init_heap((void*)heap_begin, heap_max_size, heap_init_size);

    interval_t heap_interval;
    heap_interval.begin = heap_begin;
    heap_interval.end   = heap_begin + heap_max_size;

    // Handle the virtual allocator 
    init_virt_region();
    kvregion_mark(
        (void*)KERNEL_VIRT_ADDR, 
        div_up(kernel_size, PAGE_SIZE),
        VREGION_KERNELIMG,
        "Kernel"
    );
    kvregion_mark(
        (void*)pfn_interval.begin, 
        div_up(pfn_interval.end - pfn_interval.begin, PAGE_SIZE),
        VREGION_PFN,
        "Page Descriptors"
    );
    kvregion_mark(
        (void*)heap_interval.begin, 
        div_up(heap_interval.end - heap_interval.begin, PAGE_SIZE),
        VREGION_HEAP, 
        "Heap"
    );
}