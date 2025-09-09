#include <memory/phys_alloc/pfn_alloc.h>
#include <kernel/memory/virt_alloc.h>

phys_page_descriptor_t* page_desc_free_ll;

void* alloc_phys_page_pfn(enum phys_page_type page_type, uint16_t page_flags)
{
    if (!page_desc_free_ll)
    {
        return NULL;
    }

    if (page_type == PAGETYPE_UNUSED)
    {
        return NULL;
    }

    uintptr_t page_index = page_desc_free_ll - pfn_data.descs;
    
    if (page_desc_free_ll->u.free_page.count > 1)
    {
        uintptr_t end_page_index = page_index + page_desc_free_ll->u.free_page.count - 1;

        phys_page_descriptor_t* res_page_desc = &pfn_data.descs[end_page_index];
        res_page_desc->type = page_type;
        res_page_desc->flags = page_flags;
        res_page_desc->ref_count = 1;

        page_desc_free_ll->u.free_page.count--;

        uintptr_t new_foot_page_index = end_page_index - 1;
        phys_page_descriptor_t* new_foot = &pfn_data.descs[new_foot_page_index];
        new_foot->u.free_page.count = page_desc_free_ll->u.free_page.count;

        return (void*)(end_page_index  * PAGE_SIZE);
    }

    phys_page_descriptor_t* next = page_desc_free_ll->u.free_page.next_desc;
    page_desc_free_ll = next;
    if (next)
    {
        next->u.free_page.prev_desc = NULL;
    }

    phys_page_descriptor_t* res_page_desc =&pfn_data.descs[page_index];
    res_page_desc->type = page_type;
    res_page_desc->flags = page_flags;
    res_page_desc->ref_count = 1;

    return (void*)(page_index * PAGE_SIZE);
}

void free_phys_page_pfn(void* addr_ptr)
{   
    uintptr_t addr = (uintptr_t)addr_ptr;

    assert((addr & (PAGE_SIZE - 1)) == 0);
    assert(addr < max_memory);

    uintptr_t page_index = addr / PAGE_SIZE;
    phys_page_descriptor_t* cur_desc = &pfn_data.descs[page_index];
    
    assert(cur_desc->type != PAGETYPE_UNUSED);
    assert(cur_desc->ref_count > 0);

    if (--cur_desc->ref_count > 0) 
        return; 

    cur_desc->type = PAGETYPE_UNUSED;

    phys_page_descriptor_t* old_foot = (page_index > 0) ? 
            &pfn_data.descs[page_index-1] : NULL;
    if (old_foot && old_foot->type != PAGETYPE_UNUSED)
        old_foot = NULL;

    phys_page_descriptor_t* old_head = (page_index + 1 < pfn_data.count) ? 
            &pfn_data.descs[page_index+1] : NULL;
    if (old_head && old_head->type != PAGETYPE_UNUSED)
        old_head = NULL;

    // back desc is foot
    if (old_foot && !old_head)
    {
        // back desc is head
        phys_page_descriptor_t* new_head = &pfn_data.descs[page_index - old_foot->u.free_page.count];
        phys_page_descriptor_t* new_foot = cur_desc;

        cur_desc->u.free_page.prev_desc = NULL;
        cur_desc->u.free_page.next_desc = NULL;

        uintptr_t new_count = new_head->u.free_page.count + 1;
        
        new_head->u.free_page.count = new_count;
        new_foot->u.free_page.count = new_count;

        return;
    }
    // next desc is head
    else if (!old_foot && old_head)
    {
        phys_page_descriptor_t* new_foot = &pfn_data.descs[page_index + old_head->u.free_page.count];
        phys_page_descriptor_t* new_head = cur_desc;

        uintptr_t new_count = old_head->u.free_page.count + 1;

        new_foot->u.free_page.count = new_count; 
        new_head->u.free_page.count = new_count;

        phys_page_descriptor_t* next = old_head->u.free_page.next_desc;
        phys_page_descriptor_t* prev = old_head->u.free_page.prev_desc;

        new_head->u.free_page.next_desc = next;
        new_head->u.free_page.prev_desc = prev;

        if (next)
            next->u.free_page.prev_desc = new_head;

        if (prev)
            prev->u.free_page.next_desc = new_head;
        else
            page_desc_free_ll = new_head;

        return;
    }
    // neither is head or foot
    else if (!old_foot && !old_head)
    {
        cur_desc->u.free_page.count = 1;
        
        cur_desc->u.free_page.prev_desc = NULL;
        cur_desc->u.free_page.next_desc = page_desc_free_ll;

        if (page_desc_free_ll)
            page_desc_free_ll->u.free_page.prev_desc = cur_desc;

        page_desc_free_ll = cur_desc;

        return;
    }

    // there is a head and foot
    
    phys_page_descriptor_t* new_head = 
        &pfn_data.descs[page_index - old_foot->u.free_page.count];

    phys_page_descriptor_t* new_foot = 
        &pfn_data.descs[page_index + old_head->u.free_page.count];

    // update count
    uintptr_t new_count = old_foot->u.free_page.count + old_head->u.free_page.count + 1; 
    new_head->u.free_page.count = new_count;
    new_foot->u.free_page.count = new_count;

    // remove old_head from the list
    phys_page_descriptor_t* old_head_prev = old_head->u.free_page.prev_desc;
    phys_page_descriptor_t* old_head_next = old_head->u.free_page.next_desc;
 
    if (old_head_prev)
    {
        old_head_prev->u.free_page.next_desc = old_head_next;
    }
    else
    {
        page_desc_free_ll = old_head_next;
    }

    if (old_head_next)
    {
        old_head_next->u.free_page.prev_desc = old_head_prev;
    }
}

// will be not used for now, maybe next version will add free/allocated memory regions
/*
static void free_page_desc_region(void* start_ptr, uintptr_t count)
{
    uintptr_t end   = (uintptr_t) (start_ptr + count * PAGE_SIZE);
    uintptr_t start = (uintptr_t) start_ptr;
    uintptr_t page_index     = start / PAGE_SIZE;
    uintptr_t page_index_end = end   / PAGE_SIZE;
    uintptr_t total_pages    = pfn_data.count; 

    if (page_index >= total_pages) 
    {
        return;
    }
    if (page_index_end > total_pages) 
    {
        page_index_end = total_pages;
    }

    while (page_index < page_index_end)
    {
        while (page_index < page_index_end && pfn_data.descs[page_index].type != PAGETYPE_UNUSED)
        {
            page_index++;
        }
    
        if (page_index >= page_index_end)
        {
            break;
        }
        
        uintptr_t from_index = page_index;
        phys_page_descriptor_t* head = &pfn_data.descs[from_index];

        while (page_index < page_index_end && pfn_data.descs[page_index].type == PAGETYPE_UNUSED)
        {
            pfn_data.descs[page_index].u.free_page.count = 0;
            pfn_data.descs[page_index].u.free_page.prev_desc = head;
            page_index++;
        }

        uintptr_t region_count = page_index - from_index;

        phys_page_descriptor_t* foot = &pfn_data.descs[from_index + region_count - 1];

        foot->u.free_page.count = region_count;
        head->u.free_page.count = region_count;

        head->u.free_page.next_desc = page_desc_free_ll;
        head->u.free_page.prev_desc = NULL;

        page_desc_free_ll = head;
        
        if (head->u.free_page.next_desc)
        {
            head-> u.free_page.next_desc-> u.free_page.prev_desc = head;
        }
    }
}
*/

static void identity_map_page_region(void* start_pa, void* end_pa)
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
        uintptr_t start = i;
        while (i < pfn_data.count && pfn_data.descs[i].type == PAGETYPE_UNUSED) 
        {
        
            pfn_data.descs[i].u.free_page.count = 0;
            pfn_data.descs[i].u.free_page.prev_desc = &pfn_data.descs[start];
        
            ++i;
        }

        uintptr_t cnt = i - start;
        
        if (cnt == 0) 
            continue;

        phys_page_descriptor_t* head = &pfn_data.descs[start];
        phys_page_descriptor_t* foot = &pfn_data.descs[start + cnt - 1];

        // Fix head/foot
        head->u.free_page.count = cnt;
        foot->u.free_page.count = cnt;

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

    boot_foreach_reserved_region(boot_data, identity_map_page_region);
    
    // builds the free list based on each page couple
    build_free_list();
}