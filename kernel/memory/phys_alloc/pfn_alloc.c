#include "arch/i386/memory/paging_utils.h"
#include "memory/core/pfn_desc.h"
#include "memory/heap/heap.h"
#include <memory/phys_alloc/pfn_alloc.h>
#include <kernel/memory/paging.h>
#include <stddef.h>
#include "core/num_defs.h"
#include <stdio.h>

phys_page_descriptor_t* page_desc_free_ll;

void* pfn_alloc_phys_page()
{
    phys_alloc_t alloc = pfn_alloc_phys_pages(1);

    return alloc.count ? alloc.addr : NULL;
}

static inline void mark_pages(phys_page_descriptor_t* begin, 
                       phys_page_descriptor_t* end, 
                       enum phys_page_type type, u16 flags, 
                       usize_ptr ref_count) 
{
    for (phys_page_descriptor_t* it = begin;  it < end; it++)
    {
        it->type  = type;
        it->flags = flags;
        it->ref_count = ref_count;
    }
} 

phys_alloc_t pfn_alloc_phys_pages(usize_ptr request_count)
{
    if (!page_desc_free_ll)
    {
        return (phys_alloc_t){
            .addr = NULL,
            .count = 0
        };
    }

    usize_ptr page_index = page_desc_free_ll - pfn_data.descs;
    usize_ptr taken_count = min(page_desc_free_ll->u.free_page.count, request_count);
    
    // Just decrease count
    if (page_desc_free_ll->u.free_page.count > taken_count)
    {
        usize_ptr result_page_index = page_index + page_desc_free_ll->u.free_page.count - taken_count;

        // Mark
        phys_page_descriptor_t* result_begin_desc = &pfn_data.descs[result_page_index];
        phys_page_descriptor_t* result_end_desc = &pfn_data.descs[page_index + page_desc_free_ll->u.free_page.count];
        mark_pages(
            result_begin_desc, result_end_desc, 
            PAGETYPE_NONE, PAGEFLAG_NONE, 
            0 // not yet mapped
        );        

        // Update list entry
        page_desc_free_ll->u.free_page.count -= taken_count;

        usize_ptr new_foot_page_index = result_page_index - 1;
        phys_page_descriptor_t* new_foot = &pfn_data.descs[new_foot_page_index];
        new_foot->u.free_page.count = page_desc_free_ll->u.free_page.count;


        return (phys_alloc_t){
            .addr  = (void*)(result_page_index  * PAGE_SIZE),
            .count = taken_count
        };
    }
    // Remove the one that was fully utilized
    else 
    {
        // Mark
        phys_page_descriptor_t* result_begin_desc = &pfn_data.descs[page_index];
        phys_page_descriptor_t* result_end_desc   = &pfn_data.descs[page_index + page_desc_free_ll->u.free_page.count];
        mark_pages(
            result_begin_desc, result_end_desc, 
            PAGETYPE_NONE, PAGEFLAG_NONE, 
            0 // not yet mapped
        );


        // Detach from free list
        phys_page_descriptor_t* next = page_desc_free_ll->u.free_page.next_desc;
        page_desc_free_ll = next;
        if (next)
        {
            next->u.free_page.prev_desc = NULL;
        }
        
        
        return (phys_alloc_t){
            .addr  = (void*)(page_index * PAGE_SIZE),
            .count = taken_count
        };
    }
}

void pfn_free_phys_page(void* addr_ptr)
{   
    pfn_free_phys_pages(
        (phys_alloc_t){.addr=addr_ptr, .count=1}
    );
}

void pfn_free_phys_pages(phys_alloc_t free_params)
{
    usize_ptr start = (usize_ptr) free_params.addr;
    usize_ptr end   = start + free_params.count * PAGE_SIZE;

    usize_ptr page_index_begin = start / PAGE_SIZE;
    usize_ptr page_index_end   = end   / PAGE_SIZE;
    usize_ptr total_pages      = pfn_data.count;

    if (page_index_begin >= total_pages)
    {
        return;
    }

    if (page_index_end > total_pages)
    {
        page_index_end = total_pages;
    }

    // Mark added free pages
    mark_pages(
        &pfn_data.descs[page_index_begin], 
        &pfn_data.descs[page_index_end],
        PAGETYPE_UNUSED, 0,
        0
    );

    // Find cur_head and cur_foot (update if extensions)
    phys_page_descriptor_t* cur_head = &pfn_data.descs[page_index_begin];
    phys_page_descriptor_t* cur_foot = &pfn_data.descs[page_index_end - 1];

    phys_page_descriptor_t* old_foot = (page_index_begin > 0) ? 
            &pfn_data.descs[page_index_begin-1] : NULL;
    // Check backwards extension
    if (old_foot && old_foot->type == PAGETYPE_UNUSED)
    {
        usize_ptr old_count = old_foot->u.free_page.count;
        cur_head = &pfn_data.descs[page_index_begin - old_count];
        page_index_begin -= old_count;
    }

    phys_page_descriptor_t* old_head = (page_index_end < pfn_data.count) ? 
            &pfn_data.descs[page_index_end] : NULL;
    // Check forwards extension
    if (old_head && old_head->type == PAGETYPE_UNUSED)
    {
        usize_ptr old_count = old_head->u.free_page.count;
        cur_foot = &pfn_data.descs[page_index_end + old_count - 1];
        page_index_end += old_count;

        // unlink from linked list
        phys_page_descriptor_t* prev = old_head->u.free_page.prev_desc;
        phys_page_descriptor_t* next = old_head->u.free_page.next_desc;

        if (prev)
            prev->u.free_page.next_desc = next; 
        else
            page_desc_free_ll = next;

        if (next)
            next->u.free_page.prev_desc = prev;
    }
    
    // update count
    usize_ptr cur_count = (usize_ptr)(cur_foot - cur_head) + 1;
    cur_head->u.free_page.count = cur_count;
    cur_foot->u.free_page.count = cur_count;

    // insert in linked list
    cur_head->u.free_page.prev_desc = NULL;
    cur_head->u.free_page.next_desc = page_desc_free_ll;
    if (page_desc_free_ll)
    {
        page_desc_free_ll->u.free_page.prev_desc = cur_head;
    }
    page_desc_free_ll = cur_head;
}

static void reserve_map_page_region(void* start_pa, void* end_pa)
{
    u64 begin = (u64)(usize_ptr)start_pa;
    u64 end = (u64)(usize_ptr)end_pa;

    u16 pfn_flags = PAGEFLAG_KERNEL | PAGEFLAG_READONLY | PAGEFLAG_IDEN_MAP;

    identity_map_pages(
        (void*)begin, (end-begin)/PAGE_SIZE, 
        pfn_to_hw_flags(pfn_flags)
    );
    pfn_mark_range((void*)begin, (end-begin)/PAGE_SIZE, PAGETYPE_RESERVED, pfn_flags);
}


static void build_free_list()
{
    page_desc_free_ll = NULL;

    usize_ptr i = 0;
    while (i < pfn_data.count) 
    {
        while (i < pfn_data.count && pfn_data.descs[i].type != PAGETYPE_UNUSED) 
        {
            pfn_data.descs[i].u.free_page.count     = 0;
            pfn_data.descs[i].u.free_page.prev_desc = NULL;
            pfn_data.descs[i].u.free_page.next_desc = NULL;
            ++i;
        }
        
        if (i >= pfn_data.count) 
            break;

        // Start of a free run
        usize_ptr run_start = i;
        while (i < pfn_data.count && pfn_data.descs[i].type == PAGETYPE_UNUSED) 
        {
            ++i;
        }
        
        usize_ptr count = i - run_start;
        
        if (count == 0) 
            continue;

        phys_page_descriptor_t* head = &pfn_data.descs[run_start];
        phys_page_descriptor_t* foot = &pfn_data.descs[run_start + count - 1];

        // Fix head/foot
        head->u.free_page.count = count;
        foot->u.free_page.count = count;

        // push head list 
        head->u.free_page.prev_desc = NULL;
        head->u.free_page.next_desc = page_desc_free_ll;
        if (page_desc_free_ll)
            page_desc_free_ll->u.free_page.prev_desc = head;
        page_desc_free_ll = head;
    }
}

phys_run_vec_t pfn_alloc_phys_run_vector(usize_ptr count)
{
    phys_run_vec_t vector = {
        .runs = NULL,
        .run_count = 0,
        .total_pages = 0
    };

    while (vector.total_pages< count)
    {
        usize cur_index = vector.run_count;
        vector.run_count++;

        vector.runs = krealloc(vector.runs, sizeof(phys_alloc_t) * vector.run_count);
        
        vector.runs[cur_index] = pfn_alloc_phys_pages(count - vector.total_pages);
        vector.total_pages += vector.runs[cur_index].count;
    }

    return vector;
}

void pfn_free_phys_pages_vector(phys_run_vec_t vector)
{
    for (usize i = 0; i < vector.run_count; i++) 
    {
        free_phys_pages(vector.runs[i]);
    }
    kfree(vector.runs);
}


void init_pfn_allocator(boot_data_t* boot_data)
{
    // Make the free descriptor list
    page_desc_free_ll = NULL;

    boot_foreach_reserved_region(boot_data, reserve_map_page_region);
    
    // builds the free list based on each page couple
    build_free_list();
}