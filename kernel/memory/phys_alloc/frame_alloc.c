#include <memory/phys_alloc/frame_alloc.h>

#include "arch/i386/memory/paging_utils.h"
#include "memory/core/pfn_desc.h"
#include "memory/heap/heap.h"
#include <kernel/memory/paging.h>
#include <stddef.h>
#include "core/num_defs.h"
#include <stdio.h>

page_t* page_desc_free_ll;

static inline void pfn_mark_pages(page_t* begin, 
                       page_t* end, 
                       enum phys_page_type type, u16 flags, 
                       usize_ptr ref_count) 
{
    for (page_t* it = begin;  it < end; it++)
    {
        it->type  = type;
        it->flags = flags;
        it->ref_count = ref_count;
    }
} 

page_t* frame_alloc_phys_pages(usize_ptr request_count)
{
    page_t* it = page_desc_free_ll;
    while (it)
    {
        usize_ptr block_count = it->u.free_page.count;

        if (block_count < request_count)
        {
            it = it->u.free_page.next_desc;
            continue;
        }

        // Calculate index
        usize_ptr block_start_index = get_pfn_index(it);
        usize_ptr block_end_index   = block_start_index + block_count;

        usize_ptr alloc_start_index = block_end_index - request_count;

        // Mark
        page_t* alloc_begin = page_index_to_pfn(alloc_start_index);
        page_t* alloc_end   = page_index_to_pfn(block_end_index);

        for (page_t* it = alloc_begin; it != alloc_end; it++)
        {
            assert(it->ref_count == 0);
            assert(it->type      == PAGETYPE_UNUSED);
        }

        pfn_range_set_type(
            alloc_begin, 
            block_end_index - alloc_start_index + 1,
            PAGETYPE_USED
        );

        // Update the free run count
        usize_ptr new_count = block_count - request_count;

        if (new_count == 0)
        {
            // Detach from free list
            page_t* prev = it->u.free_page.prev_desc;
            page_t* next = it->u.free_page.next_desc;

            if (prev) 
            {
                prev->u.free_page.next_desc = next;
            }
            else
            {
                page_desc_free_ll = next;
            }
            
            if (next) 
            {
                next->u.free_page.prev_desc = prev;
            }
        }
        else 
        {
            // Update list entry
            it->u.free_page.count = new_count;
        
            page_t* new_foot = page_index_to_pfn(alloc_start_index);
            new_foot->u.free_page.count = new_count;
        }

        return page_index_to_pfn(alloc_start_index);
    }

    return NULL;
}

void frame_free_phys_pages(page_t* pfn, usize_ptr count)
{
    usize_ptr start = (usize_ptr)pfn_to_pa(pfn);
    usize_ptr end   = start + count * PAGE_SIZE;

    usize_ptr page_index_begin = start / PAGE_SIZE;
    usize_ptr page_index_end   = end   / PAGE_SIZE;
    usize_ptr total_pages      = pfn_page_count();

    if (page_index_begin >= total_pages)
    {
        return;
    }

    if (page_index_end > total_pages)
    {
        page_index_end = total_pages;
    }

    

    // Mark added free pages
    pfn_mark_pages(
        page_index_to_pfn(page_index_begin), 
        page_index_to_pfn(page_index_end), 
        PAGETYPE_UNUSED, 0,
        0
    );

    // Find cur_head and cur_foot (update if extensions)
    page_t* cur_head = page_index_to_pfn(page_index_begin);
    page_t* cur_foot = page_index_to_pfn(page_index_end - 1);

    page_t* old_foot = (page_index_begin > 0) ? 
            page_index_to_pfn(page_index_begin - 1) : NULL;
    // Check backwards extension
    if (old_foot && old_foot->type == PAGETYPE_UNUSED)
    {
        usize_ptr old_count = old_foot->u.free_page.count;
        cur_head = page_index_to_pfn(page_index_begin - old_count);
        page_index_begin -= old_count;
    }

    page_t* old_head = (page_index_end < total_pages) ? 
            page_index_to_pfn(page_index_end) : NULL;
    // Check forwards extension
    if (old_head && old_head->type == PAGETYPE_UNUSED)
    {
        usize_ptr old_count = old_head->u.free_page.count;
        cur_foot = page_index_to_pfn(page_index_end + old_count - 1);
        page_index_end += old_count;

        // unlink from linked list
        page_t* prev = old_head->u.free_page.prev_desc;
        page_t* next = old_head->u.free_page.next_desc;

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
    usize_ptr begin = (usize_ptr)start_pa;
    usize_ptr end = (usize_ptr)end_pa;

    u16 paging_flags = PAGING_FLAG_WRITE | PAGING_FLAG_PRESENT;

    paging_map_identity(
        begin, (end-begin)/PAGE_SIZE, 
        paging_flags
    );
    pfn_mark_range((void*)begin, (end-begin)/PAGE_SIZE, PAGETYPE_RESERVED, paging_flags);
}


static usize_ptr build_free_list()
{
    page_desc_free_ll = NULL;

    usize_ptr i = 0;

    usize_ptr free_count = 0;
    
    usize_ptr count = pfn_page_count();
    while (i < count) 
    {
        while (i < count && page_index_to_pfn(i)->type != PAGETYPE_UNUSED) 
        {
            page_index_to_pfn(i)->u.free_page.count     = 0;
            page_index_to_pfn(i)->u.free_page.prev_desc = NULL;
            page_index_to_pfn(i)->u.free_page.next_desc = NULL;
            ++i;
        }
        
        if (i >= count) 
            break;

        // Start of a free run
        usize_ptr run_start = i;
        while (i < count && page_index_to_pfn(i)->type == PAGETYPE_UNUSED) 
        {
            ++i;
        }
        
        usize_ptr count = i - run_start;
        
        if (count == 0) 
            continue;

        free_count += count;

        page_t* head = page_index_to_pfn(run_start);
        page_t* foot = page_index_to_pfn(run_start + count - 1);

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

    return free_count;
}

usize_ptr init_frame_allocator(boot_data_t* boot_data)
{
    // Make the free descriptor list
    page_desc_free_ll = NULL;

    boot_foreach_reserved_region(boot_data, reserve_map_page_region);
    
    // builds the free list based on each page couple
    return build_free_list();
}