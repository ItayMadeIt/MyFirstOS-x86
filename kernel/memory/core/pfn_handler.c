#include <memory/core/pfn_desc.h>
#include <memory/core/init.h>
#include "kernel/memory/paging.h"
#include "core/num_defs.h"
#include "memory/phys_alloc/bitmap_alloc.h"
#include "memory/phys_alloc/frame_alloc.h"
#include <kernel/core/paging.h>
#include <memory/core/pfn_desc.h>
#include <kernel/memory/paging.h>
#include <kernel/core/cpu.h>
#include <memory/core/linker_vars.h>
#include <stdio.h>
#include <string.h>

#define INIT_SIZE STOR_16KiB

static pfn_manager_data_t pfn_data;

static usize_ptr early_alloc_and_map(
    void* start_va,
    usize_ptr total_pages,
    u16 paging_flags)
{
    usize_ptr mapped = 0;
    u8* cur_va = (u8*)start_va;

    while (mapped < total_pages)
    {
        usize_ptr remaining = total_pages - mapped;

        phys_alloc_t phys_run = bitmap_alloc_pages(remaining);
        if (!phys_run.count)
        {
            printf("early_alloc_and_map: out of physical pages\n");
            cpu_halt();
        }

        paging_map_pages(
            phys_run.addr, cur_va, 
            phys_run.count, 
            paging_flags
        );

        mapped += phys_run.count;
        cur_va += phys_run.count * PAGE_SIZE;
    }

    return mapped;
}

typedef struct bump_vars
{
    usize_ptr max_addr;
    usize_ptr alloc_addr;
    usize_ptr begin_addr;
} bump_vars_t;

static bump_vars_t bump;

static void* bump_alloc_align(const usize_ptr size, usize_ptr alignment)
{
    if (alignment < sizeof(void*)) 
    {
        alignment = sizeof(void*);
    }
    alignment = align_up_pow2(alignment);
    
    usize_ptr aligned_addr = (bump.alloc_addr + alignment - 1) & ~(alignment - 1);
    if (aligned_addr > UINT32_MAX - size) 
    { 
        printf("bump: overflow\n"); 
        cpu_halt(); 
    }
    
    usize_ptr needed_end = aligned_addr + size;

    // Allocate more pages
    if (needed_end > bump.max_addr) 
    {
        usize_ptr new_max = round_page_up(needed_end); 
        usize_ptr delta   = (new_max - bump.max_addr);

        assert(early_alloc_and_map(
            (void*)bump.max_addr, delta/PAGE_SIZE, 
            PAGING_FLAG_WRITE | PAGING_FLAG_READ
        ));

        bump.max_addr = new_max;
    }

    bump.alloc_addr = aligned_addr + size;

    return (void*)aligned_addr;
}

static void init_bump(void* begin_addr_va)
{
    usize_ptr pages_count = ((usize_ptr)round_page_up(INIT_SIZE)/PAGE_SIZE);

    assert(early_alloc_and_map(
        begin_addr_va, pages_count, 
        PAGING_FLAG_WRITE | PAGING_FLAG_READ
        )
    );

    bump.begin_addr = (usize_ptr)begin_addr_va;
    bump.alloc_addr = (usize_ptr)begin_addr_va;
    bump.max_addr = (usize_ptr)begin_addr_va + pages_count*PAGE_SIZE;
}

void pfn_set_type(page_t* desc, enum phys_page_type type)
{
    desc->type = type;
}

void pfn_range_set_type    (page_t* desc, usize_ptr count, enum phys_page_type type)
{
    page_t* it = desc;

    for (usize_ptr i = 0; i < count; ++i) 
    {
        it->type = type;
        it++;
    }
}


page_t* page_index_to_pfn(usize_ptr pfn_index)
{
    return &pfn_data.descs[pfn_index];
}

page_t *pa_to_pfn(void *phys_addr)
{
    if (!pfn_data.descs)
        return NULL;

    usize_ptr page_index = (usize_ptr)phys_addr/PAGE_SIZE;

    if (page_index < pfn_data.count)
    {
        return &pfn_data.descs[page_index];
    }
    else 
    {
        return NULL;
    }
}

void* pfn_to_pa(page_t* desc)
{
    return (void*)( PAGE_SIZE * get_pfn_index(desc) );
}

usize_ptr   get_pfn_index(page_t* desc)
{
    return (desc - pfn_data.descs);
}

static void pfn_reset_free()
{
    pfn_data.free_count = 0;
}

page_t *virt_to_pfn(void *addr)
{
    usize_ptr page_index = (usize_ptr)virt_to_phys(addr)/PAGE_SIZE;

    assert(page_index < pfn_data.count);

    return &pfn_data.descs[page_index];
}

static page_t* mark_range(
        usize_ptr start_pa, usize_ptr inclusive_end_pa,
        enum phys_page_type type, u32 ref_count, u16 flags)
{
    assert(inclusive_end_pa > start_pa);

    usize_ptr start = start_pa / PAGE_SIZE;
    usize_ptr end = (inclusive_end_pa - 1) / PAGE_SIZE; // inclusive
    if (start >= pfn_data.count)
    { 
        return NULL;
    }
    if (end >= pfn_data.count) 
    {
        end = pfn_data.count - 1;
    }

    for (usize_ptr i = start; i <= end; ++i) 
    {
        pfn_data.descs[i].type = type;
        pfn_data.descs[i].ref_count = ref_count;
        pfn_data.descs[i].flags = flags;
    }
    
    return &pfn_data.descs[start];
}

static void mark_page_structure(void* page_struct_pa)
{
    u32 page_index = ((u32)page_struct_pa)/PAGE_SIZE;
    pfn_data.descs[page_index].type = PAGETYPE_VENTRY;
}

static void mark_range_usable(void* start_pa, void* end_pa)
{
    mark_range(
        (usize_ptr)start_pa,
        (usize_ptr)end_pa,
        PAGETYPE_UNUSED, 0,
        PAGEFLAG_KERNEL | PAGEFLAG_HEAP_FREE
    );
}
static void mark_range_reserved(void* start_pa, void* end_pa)
{
    mark_range(
        (usize_ptr)start_pa,
        (usize_ptr)end_pa,
        PAGETYPE_RESERVED, 0,
        PAGEFLAG_KERNEL | PAGEFLAG_READONLY
    );
}

void pfn_mark_range(void* start_pa_ptr, usize_ptr count, enum phys_page_type page_type, u16 page_flags)
{
    usize_ptr start_pa = (usize_ptr) start_pa_ptr;

    mark_range(
        start_pa, start_pa+count*PAGE_SIZE, 
        page_type, 1, page_flags
    );
}

void pfn_identity_range( 
    void* pa,
    usize_ptr count,
    enum phys_page_type pfn_type,
    u16 pfn_flags)
{
    mark_range(
        (usize_ptr)pa, 
        (usize_ptr)pa + count - PAGE_SIZE, 
        pfn_type, 
        1, pfn_flags
    );
}

void pfn_mark_vrange(void* start_va_ptr, usize_ptr count, enum phys_page_type page_type, u16 page_flags)
{
    usize_ptr start_va = (usize_ptr) start_va_ptr;

    usize_ptr run_pa_start = 0;
    usize_ptr run_pa_count = 0;
    
    for (usize_ptr i = 0; i < count; i++) 
    {
        usize_ptr cur_pa = (usize_ptr) virt_to_phys((void*) (start_va + PAGE_SIZE * i));

        if (run_pa_count == 0)
        {
            run_pa_start = cur_pa;
            run_pa_count = 1;
        }
        else if (run_pa_start + PAGE_SIZE * run_pa_count == cur_pa)
        {
            run_pa_count++;
        }
        else
        {
            mark_range(
                run_pa_start, run_pa_start+run_pa_count*PAGE_SIZE, 
                page_type, 1, page_flags
            );
            
            run_pa_start = cur_pa;
            run_pa_count = 1;
        }
    }


    if (run_pa_count > 0)
    {
        mark_range(
            run_pa_start, run_pa_start + run_pa_count * PAGE_SIZE,
            page_type, 1, page_flags
        );
    }
}


static void init_phys_pages(boot_data_t* boot_data)
{
    // Mark everything as RESERVERD
    mark_range(
        0, (pfn_data.count - 1) * PAGE_SIZE,
        PAGETYPE_RESERVED, 1, 0
    );

    boot_foreach_free_page_region(boot_data, mark_range_usable);
    boot_foreach_reserved_region(boot_data, mark_range_reserved);

    // Kernel image
    mark_range(
        round_page_down(kernel_begin_pa),
        round_page_down(kernel_end_pa),
        PAGETYPE_KERNELIMG, 1, 0
    );

    // Phys page descriptors backing storage
    mark_range(
        round_page_down((u32)virt_to_phys((void*)bump.begin_addr)),
        round_page_down((u32)virt_to_phys((void*)bump.alloc_addr-PAGE_SIZE)), // inclusive
        PAGETYPE_PFN, 1, 0
    );

    boot_foreach_page_struct(mark_page_structure);
}

void pfn_ref_page(page_t* desc)
{
    assert(desc);

    desc->ref_count++;
}
bool pfn_unref_page(page_t* desc)
{
    assert(desc);

    assert(desc->ref_count > 0);

    desc->ref_count--;

    return desc->ref_count == 0;
}

void pfn_ref_range(page_t* desc, usize_ptr count)
{    
    for (usize_ptr i = 0; i < count; i++) 
    {
        assert(desc);

        pfn_ref_page(desc);

        desc++;
    }
}

usize_ptr pfn_unref_range(page_t* desc, usize_ptr count)
{
    usize_ptr free_count = 0;

    for (usize_ptr i = 0; i < count; i++)
    {
        assert(desc);
        assert (desc->ref_count != 0);

        free_count += pfn_unref_page(desc) ? 1 : 0;

        desc++;
    }
    
    return free_count;
}

usize_ptr pfn_page_count()
{
    return pfn_data.count;
}

usize_ptr pfn_page_free_count()
{
    return pfn_data.free_count;
}

void pfn_free_amount(usize_ptr amount)
{
    pfn_data.free_count += amount;
    assert(pfn_data.free_count < pfn_data.count);
}
void pfn_alloc_amount(usize_ptr amount)
{
    assert(pfn_data.free_count > amount);

    pfn_data.free_count -= amount;
}

void init_pfn_descriptors(void** alloc_addr, boot_data_t* boot_data)
{
    pfn_data.count = 0;
    pfn_data.descs = NULL;
    
    pfn_reset_free();

    init_bump(*alloc_addr);

    pfn_data.count = max_memory / PAGE_SIZE;
    pfn_data.descs = bump_alloc_align(
        pfn_data.count * sizeof(page_t), 
        0
    );

    memset(pfn_data.descs, 0, pfn_data.count * sizeof(page_t));

    init_phys_pages(boot_data);
    
    pfn_data.free_count = init_frame_allocator(boot_data);

    *alloc_addr = (void*)bump.alloc_addr;
}