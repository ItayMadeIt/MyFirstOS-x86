#include "arch/i386/core/paging.h"
#include "arch/i386/memory/paging_utils.h"
#include "memory/phys_alloc/pfn_alloc.h"
#include "memory/phys_alloc/phys_alloc.h"
#include <memory/core/pfn_desc.h>
#include <kernel/core/paging.h>
#include <kernel/memory/paging.h>
#include <core/defs.h>
#include <kernel/core/cpu.h>
#include <memory/core/linker_vars.h>
#include "core/num_defs.h"
#include <stdio.h>
#include <string.h>

#define INIT_SIZE STOR_16KiB


static usize_ptr early_alloc_and_map(
    void* start_va,
    usize_ptr total_pages,
    u16 pfn_flags)
{
    usize_ptr mapped = 0;
    usize_ptr hw_flags = pfn_to_hw_flags(pfn_flags);
    u8* cur_va = (u8*)start_va;

    while (mapped < total_pages)
    {
        usize_ptr remaining = total_pages - mapped;

        phys_alloc_t phys_run = alloc_phys_pages(remaining);
        if (!phys_run.count)
        {
            printf("early_alloc_and_map: out of physical pages\n");
            cpu_halt();
        }

        map_phys_pages(
            phys_run.addr, cur_va, 
            phys_run.count, 
            hw_flags
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
            PAGEFLAG_KERNEL
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
        PAGEFLAG_KERNEL
        )
    );

    bump.begin_addr = (usize_ptr)begin_addr_va;
    bump.alloc_addr = (usize_ptr)begin_addr_va;
    bump.max_addr = (usize_ptr)begin_addr_va + pages_count*PAGE_SIZE;
}

pfn_manager_data_t pfn_data;

phys_page_descriptor_t *phys_to_pfn(void *phys_addr)
{
    if (!pfn_data.descs)
        return NULL;

    usize_ptr page_index = (usize_ptr)phys_addr/PAGE_SIZE;

    assert(page_index < pfn_data.count);

    return &pfn_data.descs[page_index];
}

phys_page_descriptor_t *virt_to_pfn(void *addr)
{
    usize_ptr page_index = (usize_ptr)virt_to_phys(addr)/PAGE_SIZE;

    assert(page_index < pfn_data.count);

    return &pfn_data.descs[page_index];
}

static inline phys_page_descriptor_t* mark_range(usize_ptr start_pa, usize_ptr inclusive_end_pa,
                              phys_page_descriptor_t* pages, usize_ptr total_pages,
                              enum phys_page_type type, u32 ref_count, u16 flags)
{
    assert(inclusive_end_pa > start_pa);

    usize_ptr start = start_pa / PAGE_SIZE;
    usize_ptr end = (inclusive_end_pa - 1) / PAGE_SIZE; // inclusive
    if (start >= total_pages)
    { 
        return NULL;
    }
    if (end >= total_pages) 
    {
        end = total_pages - 1;
    }

    for (usize_ptr i = start; i <= end; ++i) 
    {
        pages[i].type = type;
        pages[i].ref_count = ref_count;
        pages[i].flags = flags;
    }
    
    return &pages[start];
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
        pfn_data.descs, pfn_data.count,
        PAGETYPE_UNUSED, 0,
        PAGEFLAG_KERNEL | PAGEFLAG_VFREE
    );
}
static void mark_range_reserved(void* start_pa, void* end_pa)
{
    mark_range(
        (usize_ptr)start_pa,
        (usize_ptr)end_pa,
        pfn_data.descs, pfn_data.count,
        PAGETYPE_RESERVED, 0,
        PAGEFLAG_KERNEL | PAGEFLAG_READONLY
    );
}

void pfn_mark_range(void* start_pa_ptr, usize_ptr count, enum phys_page_type page_type, u16 page_flags)
{
    usize_ptr start_pa = (usize_ptr) start_pa_ptr;

    mark_range(
        start_pa, start_pa+count*PAGE_SIZE, 
        pfn_data.descs, pfn_data.count, 
        page_type, 0, page_flags
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
                pfn_data.descs, pfn_data.count, 
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
            pfn_data.descs, pfn_data.count,
            page_type, 1, page_flags
        );
    }
}

void pfn_alloc_map_pages(
    void* va,
    usize_ptr count, 
    enum phys_page_type pfn_type,
    u16 pfn_flags)
{
    usize_ptr cur_count = 0;
    usize_ptr cur_va = (usize_ptr)va;
    while (cur_count < count)
    {
        phys_alloc_t alloc = pfn_alloc_phys_pages(count - cur_count);

        assert(map_phys_pages(
                alloc.addr, (void*)cur_va, 
                alloc.count,
                    pfn_to_hw_flags(pfn_flags)
        ) );

        cur_va += PAGE_SIZE * alloc.count;
        cur_count += alloc.count;
    }   

    pfn_mark_vrange(va, count, pfn_type, pfn_flags); 
}

void pfn_alloc_map_page (
    void* va,
    enum phys_page_type pfn_type,
    u16 pfn_flags)
{
    pfn_alloc_map_pages(va, 1, pfn_type, pfn_flags);
}


void pfn_identity_map_pages(
    void* pa, 
    usize_ptr count,
    enum phys_page_type pfn_type,
    u16 pfn_flags)
{
    void* va = pa;
    assert(identity_map_pages(
            pa,
            count, 
            pfn_to_hw_flags(pfn_flags)
    ));    

    pfn_mark_vrange(va, count, pfn_type, pfn_flags);
}
void pfn_map_pages(
    void* pa,
    void* va,
    usize_ptr count,
    enum phys_page_type type,
    u16 flags)
{
    
    u16 hw_flags = pfn_to_hw_flags(flags);

    // Step 1: hardware map
    assert(map_phys_pages(
        pa, va, count, hw_flags
    ));

    mark_range((usize_ptr)pa, (usize_ptr)pa + PAGE_SIZE * (count-1), 
        pfn_data.descs, pfn_data.count, 
        type, 1, flags);
}

void pfn_unmap_pages(
    void* va, 
    usize_ptr count)
{
    pfn_unref_vrange(va, count);
    unmap_pages(va, count);
}


void pfn_unmap_page (void* va)
{
    pfn_unref_vpage(va);
    unmap_page(va);
}

void pfn_clone_map(
    void* dst_va,
    void* src_va,
    usize_ptr count)
{
    assert(dst_va && src_va);
    assert(count > 0);

    u8* dst = (u8*)dst_va;
    u8* src = (u8*)src_va;

    for (usize_ptr i = 0; i < count; i++)
    {
        void* pa = (void*)virt_to_phys(src);
        assert(pa);

        phys_page_descriptor_t* desc = phys_to_pfn(pa);
        assert(desc);
        desc->ref_count++;

        u16 hw_flags = pfn_to_hw_flags(desc->flags);
        map_phys_page(pa, dst, hw_flags);

        dst += PAGE_SIZE;
        src += PAGE_SIZE;
    }
}

void pfn_share_map(
    void* dst_va,
    void* src_va,
    usize_ptr count,
    u16 dst_pfn_flags)
{
    assert(dst_va && src_va);
    assert(count > 0);

    u16 dst_hw_flags = pfn_to_hw_flags(dst_pfn_flags);

    u8* dst = (u8*)dst_va;
    u8* src = (u8*)src_va;

    for (usize_ptr i = 0; i < count; i++)
    {
        void* pa = (void*)virt_to_phys(src);
        assert(pa);

        phys_page_descriptor_t* desc = phys_to_pfn(pa);
        assert(desc);
        desc->ref_count++;

        map_phys_page(pa, dst, dst_hw_flags);

        dst += PAGE_SIZE;
        src += PAGE_SIZE;
    }
}

static void init_phys_pages(boot_data_t* boot_data)
{
    // Mark everything as RESERVERD
    mark_range(0, (pfn_data.count - 1) * PAGE_SIZE,
               pfn_data.descs, pfn_data.count, 
               PAGETYPE_RESERVED, 1, 0);

    boot_foreach_free_page_region(boot_data, mark_range_usable);
    boot_foreach_reserved_region(boot_data, mark_range_reserved);

    // Kernel image
    mark_range(round_page_down(kernel_begin_pa),
               round_page_down(kernel_end_pa),
               pfn_data.descs, pfn_data.count, 
               PAGETYPE_KERNEL, 1, 0);

    // Phys page descriptors backing storage
    mark_range(round_page_down((u32)virt_to_phys((void*)bump.begin_addr)),
               round_page_down((u32)virt_to_phys((void*)bump.alloc_addr-PAGE_SIZE)), // inclusive
               pfn_data.descs, pfn_data.count, 
               PAGETYPE_KERNEL, 1, 0);

    boot_foreach_page_struct(mark_page_structure);
}

void pfn_ref_page(void* pa)
{
    pfn_ref_range(pa, 1);
}
void pfn_unref_page(void* pa)
{
    pfn_unref_range(pa, 1);
}
void pfn_ref_range(void* pa, usize_ptr count)
{
    for (usize_ptr i = 0; i < count; i++) 
    {
        phys_page_descriptor_t* desc = phys_to_pfn(pa);
        assert(desc);
        assert(desc->ref_count != 0);

        desc->ref_count++;

        pa += PAGE_SIZE;
    }
}
void pfn_unref_range(void* pa, usize_ptr count)
{
    void* run_start = NULL;
    usize_ptr run_count = 0;

    for (usize_ptr i = 0; i < count; i++, pa += PAGE_SIZE)
    {
        phys_page_descriptor_t* desc = phys_to_pfn(pa);
        assert(desc);
        assert (desc->ref_count != 0);

        desc->ref_count--;

        // Start or continue a free run
        if (desc->ref_count == 0)
        {
            if (!run_start)
            {
                run_start = pa;
                run_count = 1;
            }
            else if ((usize_ptr)pa == (usize_ptr)run_start + run_count * PAGE_SIZE)
            {
                run_count++;
            }
            else
            {
                pfn_free_phys_pages(
                    (phys_alloc_t){ 
                        .addr = run_start, 
                        .count = run_count 
                    }
                );
                run_start = pa;
                run_count = 1;
            }
        }
        // flush current run
        else if (run_start)
        {
            pfn_free_phys_pages((phys_alloc_t){ .addr = run_start, .count = run_count });
            run_start = NULL;
            run_count = 0;
        }
    }

    // flush any remaining run
    if (run_start)
    {
        pfn_free_phys_pages((phys_alloc_t){ .addr = run_start, .count = run_count });
    }
}

void pfn_ref_vpage(void* va)
{
    return pfn_ref_vrange(va, 1);
}

void pfn_unref_vpage(void* va)
{
    return pfn_unref_vrange(va, 1);
}

void pfn_ref_vrange(void* va, usize_ptr count)
{
    usize_ptr start_va = (usize_ptr)va;
    usize_ptr run_pa_start = 0;
    usize_ptr run_pa_count = 0;

    for (usize_ptr i = 0; i < count; i++)
    {
        usize_ptr cur_pa = (usize_ptr)virt_to_phys((void*)(start_va + PAGE_SIZE * i));

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
            pfn_ref_range((void*)run_pa_start, run_pa_count);

            run_pa_start = cur_pa;
            run_pa_count = 1;
        }
    }

    if (run_pa_count > 0)
    {
        pfn_ref_range((void*)run_pa_start, run_pa_count);
    }
}

void pfn_unref_vrange(void* va, usize_ptr count)
{
    usize_ptr start_va = (usize_ptr)va;
    usize_ptr run_pa_start = 0;
    usize_ptr run_pa_count = 0;

    for (usize_ptr i = 0; i < count; i++)
    {
        usize_ptr cur_pa = (usize_ptr)virt_to_phys((void*)(start_va + PAGE_SIZE * i));

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
            pfn_unref_range((void*)run_pa_start, run_pa_count);
            run_pa_start = cur_pa;
            run_pa_count = 1;
        }
    }

    if (run_pa_count > 0)
    {
        pfn_unref_range((void*)run_pa_start, run_pa_count);
    }
}


void init_pfn_descriptors(void** alloc_addr, boot_data_t* boot_data)
{
    pfn_data.count = 0;
    pfn_data.descs = NULL;
    
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