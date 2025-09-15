#include "core/debug.h"
#include "memory/phys_alloc/bitmap_alloc.h"
#include <memory/core/pfn_desc.h>
#include <kernel/core/paging.h>
#include <kernel/memory/virt_alloc.h>
#include <core/defs.h>
#include <kernel/core/cpu.h>
#include <memory/core/linker_vars.h>
#include <string.h>

#define INIT_SIZE STOR_16KiB

typedef struct bump_vars
{
    uintptr_t max_addr;
    uintptr_t alloc_addr;
    uintptr_t begin_addr;
} bump_vars_t;

static bump_vars_t bump;

static void* bump_alloc_align(const uintptr_t size, uintptr_t alignment)
{
    if (alignment < sizeof(void*)) 
    {
        alignment = sizeof(void*);
    }
    alignment = align_up_pow2(alignment);
    
    uintptr_t aligned_addr = (bump.alloc_addr + alignment - 1) & ~(alignment - 1);
    if (aligned_addr > UINT32_MAX - size) 
    { 
        debug_print_str("bump: overflow\n"); 
        cpu_halt(); 
    }
    
    uintptr_t needed_end = aligned_addr + size;

    // Allocate more pages
    if (needed_end > bump.max_addr) 
    {
        uintptr_t new_max = round_page_up(needed_end); 
        uintptr_t delta   = (new_max - bump.max_addr);

        assert(map_pages(
            (void*)bump.max_addr, delta/PAGE_SIZE, 
            PAGETYPE_PHYS_PAGES, 
            PAGEFLAG_KERNEL
        ));

        bump.max_addr = new_max;
    }

    bump.alloc_addr = aligned_addr + size;

    return (void*)aligned_addr;
}

static void init_bump(void* begin_addr)
{
    uintptr_t pages_count = ((uintptr_t)round_page_up(INIT_SIZE)/PAGE_SIZE);

    map_pages(
        (void*)begin_addr, pages_count, 
        PAGETYPE_PHYS_PAGES, 
        PAGEFLAG_KERNEL
    );

    bump.alloc_addr = (uintptr_t)begin_addr;
    bump.begin_addr = (uintptr_t)begin_addr;
    bump.max_addr = (uintptr_t)begin_addr + pages_count*PAGE_SIZE;
}

pfn_manager_data_t pfn_data;

phys_page_descriptor_t *phys_to_pfn(void *phys_addr)
{
    uintptr_t page_index = (uintptr_t)phys_addr/PAGE_SIZE;

    assert(page_index < pfn_data.count);

    return &pfn_data.descs[page_index];
}

phys_page_descriptor_t *virt_to_pfn(void *addr)
{
    uintptr_t page_index = (uintptr_t)get_phys_addr(addr)/PAGE_SIZE;

    assert(page_index < pfn_data.count);

    return &pfn_data.descs[page_index];
}

static inline phys_page_descriptor_t* mark_range(uintptr_t start_pa, uintptr_t end_pa,
                              phys_page_descriptor_t* pages, uintptr_t total_pages,
                              enum phys_page_type type, uint32_t ref_count, uint16_t flags)
{
    if (end_pa <= start_pa) 
    {
        return NULL;
    }

    uintptr_t start = start_pa >> 12;
    uintptr_t end = (end_pa - 1) >> 12; // inclusive
    if (start >= total_pages)
    { 
        return NULL;
    }
    if (end >= total_pages) 
    {
        end = total_pages - 1;
    }

    for (uintptr_t i = start; i <= end; ++i) 
    {
        pages[i].type = type;
        pages[i].ref_count = ref_count;
        pages[i].flags = flags;
    }
    
    return &pages[start];
}

static void mark_page_structure(void* page_struct_pa)
{
    uint32_t page_index = ((uint32_t)page_struct_pa)/PAGE_SIZE;
    pfn_data.descs[page_index].type = PAGETYPE_VENTRY;
}

static void mark_range_usable(void* start_pa, void* end_pa)
{
    mark_range(
        (uint32_t)start_pa,
        (uint32_t)end_pa,
        pfn_data.descs, pfn_data.count,
        PAGETYPE_UNUSED, 0,
        PAGEFLAG_KERNEL | PAGEFLAG_VFREE
    );
}
static void mark_range_reserved(void* start_pa, void* end_pa)
{
    mark_range(
        (uint32_t)start_pa,
        (uint32_t)end_pa,
        pfn_data.descs, pfn_data.count,
        PAGETYPE_RESERVED, 1,
        PAGEFLAG_KERNEL | PAGEFLAG_READONLY
    );
}

static void init_phys_pages(boot_data_t* boot_data)
{
    // Mark everything as RESERVERD
    mark_range(0, pfn_data.count * PAGE_SIZE,
               pfn_data.descs, pfn_data.count, 
               PAGETYPE_RESERVED, 1, 0);

    boot_foreach_reserved_region(boot_data, mark_range_reserved);
    boot_foreach_free_page_region(boot_data, mark_range_usable);

    // Kernel image
    mark_range(round_page_down(kernel_begin_pa),
               round_page_up(kernel_end_pa),
               pfn_data.descs, pfn_data.count, 
               PAGETYPE_KERNEL, 1, 0);

    // Phys page descriptors backing storage
    mark_range(round_page_down((uint32_t)get_phys_addr((void*)bump.begin_addr)),
               round_page_up((uint32_t)get_phys_addr((void*)bump.alloc_addr)),
               pfn_data.descs, pfn_data.count, 
               PAGETYPE_KERNEL, 1, 0);

    boot_foreach_page_struct(mark_page_structure);
}
void init_pfn_descriptors(void** alloc_addr, boot_data_t* boot_data)
{
    init_bump(*alloc_addr);

    pfn_data.count = max_memory / PAGE_SIZE;
    pfn_data.descs = bump_alloc_align(
        pfn_data.count * sizeof(phys_page_descriptor_t), 
        0
    );

    memset(pfn_data.descs, 0, pfn_data.count * sizeof(phys_page_descriptor_t));

    init_phys_pages(boot_data);

    *alloc_addr = (void*)bump.alloc_addr;
}