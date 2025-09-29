#include "memory/core/pfn_desc.h"
#include <memory/phys_alloc/pfn_alloc.h>
#include <kernel/memory/paging.h>
#include <stdio.h>

phys_page_descriptor_t* page_desc_free_ll;

void* alloc_phys_page_pfn()
{
    phys_alloc_t alloc = alloc_phys_pages_pfn(1);

    return alloc.count ? alloc.addr : NULL;
}

static inline void mark_pages(phys_page_descriptor_t* begin, 
                       phys_page_descriptor_t* end, 
                       enum phys_page_type type, uint16_t flags, 
                       uintptr_t ref_count) 
{
    for (phys_page_descriptor_t* it = begin;  it < end; it++)
    {
        it->type  = type;
        it->flags = flags;
        it->ref_count = ref_count;
    }
} 

phys_alloc_t alloc_phys_pages_pfn(uintptr_t request_count)
{
    if (!page_desc_free_ll)
    {
        return (phys_alloc_t){
            .addr = NULL,
            .count = 0
        };
    }

    uintptr_t page_index = page_desc_free_ll - pfn_data.descs;
    uintptr_t taken_count = min(page_desc_free_ll->u.free_page.count, request_count);
    
    // Just decrease count
    if (page_desc_free_ll->u.free_page.count > taken_count)
    {
        uintptr_t result_page_index = page_index + page_desc_free_ll->u.free_page.count - taken_count;

        // Mark
        phys_page_descriptor_t* result_begin_desc = &pfn_data.descs[result_page_index];
        phys_page_descriptor_t* result_end_desc = &pfn_data.descs[page_index + page_desc_free_ll->u.free_page.count];
        mark_pages(
            result_begin_desc, result_end_desc, 
            PAGETYPE_PHYS_ALLOC, PAGEFLAG_NONE, 
            0 // not yet mapped
        );        

        // Update list entry
        page_desc_free_ll->u.free_page.count -= taken_count;

        uintptr_t new_foot_page_index = result_page_index - 1;
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
            PAGETYPE_PHYS_ALLOC, PAGEFLAG_NONE, 
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

void free_phys_page_pfn(void* addr_ptr)
{   
    free_phys_pages_pfn(
        (phys_alloc_t){.addr=addr_ptr, .count=1}
    );
}

void free_phys_pages_pfn(phys_alloc_t free_params)
{
    uintptr_t start = (uintptr_t) free_params.addr;
    uintptr_t end   = start + free_params.count * PAGE_SIZE;

    uintptr_t page_index_begin = start / PAGE_SIZE;
    uintptr_t page_index_end   = end   / PAGE_SIZE;
    uintptr_t total_pages      = pfn_data.count;

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
        uintptr_t old_count = old_foot->u.free_page.count;
        cur_head = &pfn_data.descs[page_index_begin - old_count];
        page_index_begin -= old_count;
    }

    phys_page_descriptor_t* old_head = (page_index_end < pfn_data.count) ? 
            &pfn_data.descs[page_index_end] : NULL;
    // Check forwards extension
    if (old_head && old_head->type == PAGETYPE_UNUSED)
    {
        uintptr_t old_count = old_head->u.free_page.count;
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
    uintptr_t cur_count = (uintptr_t)(cur_foot - cur_head) + 1;
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
    uint64_t begin = (uint64_t)(uintptr_t)start_pa;
    uint64_t end = (uint64_t)(uintptr_t)end_pa;

    identity_map_pages(
        (void*)begin, (end-begin)/PAGE_SIZE, 
        PAGETYPE_RESERVED, 
        PAGEFLAG_KERNEL | PAGEFLAG_READONLY | PAGEFLAG_IDEN_MAP
    );
}


static void build_free_list()
{
    page_desc_free_ll = NULL;

    uintptr_t i = 0;
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
        uintptr_t run_start = i;
        while (i < pfn_data.count && pfn_data.descs[i].type == PAGETYPE_UNUSED) 
        {
            ++i;
        }
        
        uintptr_t count = i - run_start;
        
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

void init_pfn_allocator(boot_data_t* boot_data)
{
    // Make the free descriptor list
    page_desc_free_ll = NULL;

    boot_foreach_reserved_region(boot_data, reserve_map_page_region);
    
    // builds the free list based on each page couple
    build_free_list();
}