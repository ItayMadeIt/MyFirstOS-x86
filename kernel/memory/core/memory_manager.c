#include "core/defs.h"
#include "core/num_defs.h"
#include "string.h"
#include <memory/core/memory_manager.h>
#include <memory/core/pfn_desc.h>

#include "memory/phys_alloc/bitmap_alloc.h"
#include "memory/phys_alloc/frame_alloc.h"

#include <memory/heap/heap.h>
#include <kernel/boot/boot_data.h>
#include "memory/virt/virt_alloc.h"
#include "memory/virt/virt_region.h"
#include "memory/virt/virt_map.h"
#include <stdio.h>

#define MM_AREA_VIRT   0xD0000000
#define KERNEL_ADDR    0x00100000
#define KERNEL_VIRT_ADDR 0xC0000000
#define HEAP_MAX_SIZE  STOR_256MiB

void mm_ensure_memory(usize_ptr count)
{
    assert(pfn_page_free_count() > count);
}

void mm_reclaim_memory(usize_ptr count)
{
    assert(pfn_page_free_count() > count);
}

static enum mm_alloc_type alloc_type = ALLOC_NONE;

void mm_set_allocator_type(enum mm_alloc_type new_alloc_type)
{
    alloc_type = new_alloc_type;
}

void* mm_alloc_pagetable()
{
    switch (alloc_type) 
    {
        case ALLOC_BITMAP:
            return bitmap_alloc_page();

        case ALLOC_FRAME:
            return pfn_to_pa( frame_alloc_phys_pages(1) );

        // Can't allocate if it's not bitmap or frame
        default:
            abort();
            return NULL;
    }
}

page_t* mm_alloc_pages(usize_ptr count)
{
    page_t* result = frame_alloc_phys_pages(count);
    
    page_t* desc = result;

    for (usize_ptr i = 0; i < count; ++i) 
    {
        assert(desc->type == PAGETYPE_USED);
        desc->ref_count = 1;
        desc++;
    }
    
    return result;
}

void mm_get_page(page_t* desc)
{
    assert(desc->type != PAGETYPE_UNUSED);
    assert(desc->ref_count != 0);
    desc->ref_count++;
}

void mm_put_page(page_t* desc)
{
    assert(desc->ref_count != 0);
    desc->ref_count--;

    if (desc->ref_count == 0)
    {
        frame_free_phys_pages(
            desc, 1
        );
    }
}

void mm_get_range(page_t* begin, usize_ptr count)
{
    page_t* desc = begin;
    for (usize_ptr i = 0; i < count; ++i) 
    {
        assert(desc);
        assert(desc->ref_count != 0);
        desc->ref_count++;

        desc++;
    }
}

void mm_put_range(page_t* begin, usize_ptr count)
{
    page_t* desc = begin;

    for (usize_ptr i = 0; i < count; ++i) 
    {
        assert(desc);
        mm_put_page(desc);
        desc++;
    }
}
