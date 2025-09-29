#include "core/defs.h"
#include "memory/phys_alloc/phys_alloc.h"
#include "string.h"
#include <memory/core/memory_manager.h>
#include <memory/core/pfn_desc.h>
#include <memory/phys_alloc/bitmap_alloc.h>
#include <memory/phys_alloc/pfn_alloc.h>
#include <memory/heap/heap.h>
#include <kernel/boot/boot_data.h>
#include <memory/core/virt_alloc.h>
#include <stdio.h>

#define MM_AREA_VIRT   0xD0000000
#define KERNEL_ADDR    0x00100000
#define KERNEL_VIRT_ADDR 0xC0000000
#define HEAP_MAX_SIZE  STOR_256MiB


static phys_alloc_t dummy_alloc_phys_pages(uintptr_t count) 
{
    (void)count;
    abort();
    return (phys_alloc_t){
        .addr = NULL,
        .count = 0
    };
}

static void* dummy_alloc_phys_page() 
{
    abort();
    return NULL;
}

static void dummy_free_phys_page(void* pa) 
{
    (void)pa;
    abort();
}

static void dummy_free_phys_pages(phys_alloc_t alloc) 
{
    (void)alloc;
    abort();
}

typedef struct interval
{
    uintptr_t begin;
    uintptr_t end;
} interval_t;

void init_memory(boot_data_t* boot_data)
{
    alloc_phys_page = dummy_alloc_phys_page;
    alloc_phys_pages = dummy_alloc_phys_pages;
    free_phys_page = dummy_free_phys_page;
    free_phys_pages = dummy_free_phys_pages;

    // init a sample physical allocator
    init_bitmap_phys_allocator(boot_data);
    alloc_phys_page  = alloc_phys_page_bitmap;
    alloc_phys_pages = alloc_phys_pages_bitmap;
    free_phys_page   = free_phys_page_bitmap;
    free_phys_pages  = free_phys_pages_bitmap;

    void* free_virt_addr = (void*)MM_AREA_VIRT;

    // Make a PFN based physical allocator
    interval_t pfn_interval;
    pfn_interval.begin = (uintptr_t) free_virt_addr;
    init_pfn_descriptors(&free_virt_addr, boot_data);
    init_pfn_allocator(boot_data);
    pfn_interval.end = (uintptr_t) free_virt_addr;

    alloc_phys_page  = alloc_phys_page_pfn;
    alloc_phys_pages = alloc_phys_pages_pfn;
    free_phys_page   = free_phys_page_pfn;
    free_phys_pages  = free_phys_pages_pfn;

    uintptr_t heap_begin = round_page_up(free_virt_addr);
    uintptr_t heap_init_size = clamp(max_memory / 16, STOR_8MiB, STOR_128MiB);
    uintptr_t heap_max_size = clamp(max_memory/4, STOR_32MiB, STOR_256MiB);
    init_heap((void*)heap_begin, heap_max_size, heap_init_size);

    interval_t heap_interval;
    heap_interval.begin = heap_begin;
    heap_interval.end   = heap_begin + heap_max_size;
    
    // Handle the virtual allocator 
    init_virt_alloc();
    virt_mark_region(
        (void*)KERNEL_VIRT_ADDR, 
        (void*)(KERNEL_VIRT_ADDR + kernel_size + STOR_2MiB + STOR_128KiB),
        "Kernel"
    );
    virt_mark_region(
        (void*)pfn_interval.begin, 
        (void*)pfn_interval.end, 
        "Page Descriptors"
    );
    virt_mark_region(
        (void*)heap_interval.begin, 
        (void*)heap_interval.end, 
        "Heap"
    );
}