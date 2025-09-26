#include "arch/i386/core/paging.h"
#include "memory/core/pfn_desc.h"
#include <kernel/memory/paging.h>
#include <kernel/core/paging.h>
#include <arch/i386/memory/paging_utils.h>
#include <memory/core/memory_manager.h>
#include <string.h>


static page_table_t* ensure_page_table(uint32_t va, uint32_t flags)
{
    uint32_t dir_index = get_pde_index((void*)va);
    uint32_t entry = get_table_entry((void*)va);

    if (!(entry & PAGE_ENTRY_FLAG_PRESENT)) 
    {
        void* new_table_phys = alloc_phys_page(PAGETYPE_VENTRY, PAGE_ENTRY_FLAG_ALLPERMS);
        if (!new_table_phys) 
        {
            return NULL;
        }
        map_table_entry(new_table_phys, (void*)va, flags);
        memset((void*)(0xFFC00000 + dir_index * PAGE_SIZE), 0, PAGE_SIZE);
    }

    return get_page_table(dir_index);
}

static bool can_map_pages(void* va_ptr, uint32_t count)
{
    uint32_t va = (uint32_t)va_ptr;

    while (count) 
    {
        uint32_t dir_index   = get_pde_index((void*)va);
        uint32_t table_index = get_pte_index((void*)va);
        uint32_t pages_in_table = ENTRIES_AMOUNT - table_index;

        page_table_t* table = get_page_table(dir_index);
        // no table = can be fully utilized
        if (!table)
        {
            uint32_t skip = (pages_in_table < count) ? pages_in_table : count;
            va    += skip * PAGE_SIZE;
            count -= skip;
            continue;
        }

        // Go over each page and check if it can be allocated
        while (pages_in_table && count) 
        {
            if ((uint32_t)table->entries[table_index] & PAGE_ENTRY_FLAG_PRESENT)
            {
                return false;
            }
            va += PAGE_SIZE;
            table_index++;
            pages_in_table--;
            count--;
        }
    }

    return true;
}

static void map_phys_continous(void* pa, void* va, uint32_t count,
                         enum phys_page_type page_type, uint32_t page_flags)
{
    uint32_t hw_flags = pfn_flags_to_hw_flags(page_flags);

    for (uint32_t i = 0; i < count; i++) 
    {
        map_page_entry((uint8_t*)pa + i * PAGE_SIZE,
                       (uint8_t*)va + i * PAGE_SIZE,
                       hw_flags);
        
        pfn_map_page(
            (uint8_t*)pa + i * PAGE_SIZE, 
            page_type, page_flags
        );
    }
}

bool map_alloc_pages(void* va_ptr, uint32_t count,
                            enum phys_page_type page_type, uint32_t page_flags)
{
    uint32_t va = (uint32_t)va_ptr;

    while (count) 
    {
        uint32_t table_index = get_pte_index((void*)va);
        uint32_t pages_in_table = ENTRIES_AMOUNT - table_index;
        uint32_t chunk_count = (pages_in_table < count) ? pages_in_table : count;

        page_table_t* table = ensure_page_table(va, PAGE_ENTRY_FLAG_ALLPERMS);
        if (!table) 
        {
            return false;
        }

        phys_alloc_t alloc = alloc_phys_pages(chunk_count, page_type, page_flags);
        if (!alloc.count) 
        {
            return false;
        }

        map_phys_continous(alloc.addr, (void*)va, alloc.count, page_type, page_flags);

        va    += alloc.count * PAGE_SIZE;
        count -= alloc.count;
    }
    return true;
}

static bool map_phys_range(void* pa_ptr, void* va_ptr, uint32_t count,
                           enum phys_page_type page_type, uint32_t page_flags)
{
    uint32_t va = (uint32_t)va_ptr;
    uint32_t pa = (uint32_t)pa_ptr;

    while (count) 
    {
        uint32_t table_index = get_pte_index((void*)va);
        uint32_t pages_in_table = ENTRIES_AMOUNT - table_index;

        page_table_t* table = ensure_page_table(va, pfn_flags_to_hw_flags(page_flags));
        if (!table)
        {
            return false;
        }

        while (pages_in_table && count) 
        {
            map_page_entry((void*)pa, (void*)va, pfn_flags_to_hw_flags(page_flags));

            pfn_map_page((void*)pa, page_type, page_flags);

            va += PAGE_SIZE;
            pa += PAGE_SIZE;
            table_index++;
            pages_in_table--;
            count--;
        }
    }
    return true;
}

bool map_pages(void* va_ptr, uint32_t count,
               enum phys_page_type page_type, uint16_t page_flags)
{
    assert(((uint32_t)va_ptr % PAGE_SIZE) == 0);
    assert(count > 0);

    if (!can_map_pages(va_ptr, count)) 
    {
        return false;
    }

    return map_alloc_pages(va_ptr, count, page_type, page_flags);
}

bool map_phys_pages(void* pa_ptr, void* va_ptr, uint32_t count,
                    enum phys_page_type page_type, uint16_t page_flags)
{
    assert(((uint32_t)pa_ptr % PAGE_SIZE) == 0);
    assert(((uint32_t)va_ptr % PAGE_SIZE) == 0);
    assert(count > 0);

    return map_phys_range(pa_ptr, va_ptr, count, page_type, page_flags);
}

void* identity_map_pages(void* pa_ptr, uintptr_t count,
                         enum phys_page_type page_type, uint16_t page_flags)
{
    assert(((uint32_t)pa_ptr % PAGE_SIZE) == 0);
    assert(count > 0);

    void* va_ptr = pa_ptr;
    return can_map_pages(va_ptr, count) &&
           map_phys_range(pa_ptr, va_ptr, count, page_type, page_flags)
           ? va_ptr : NULL;
}


bool map_page(void* va, enum phys_page_type page_type, uint16_t page_flags)
{
    return map_pages(va, 1, page_type, page_flags);
}

bool map_phys_page(void* pa_ptr, void* va_ptr,
                   enum phys_page_type page_type, uint16_t page_flags)
{
    return map_phys_pages(pa_ptr, va_ptr, 1, page_type, page_flags);
}


bool unmap_page(void* va)
{
    page_table_t* table = (page_table_t*)(get_table_entry(va) & PAGE_MASK);

    uint32_t entry = get_page_entry(va);
    if (!entry || !(entry & PAGE_ENTRY_FLAG_PRESENT)) 
        return false;

    void* pa = (void*)(entry & PAGE_MASK);

    phys_page_descriptor_t* desc = phys_to_pfn(pa);
    if (desc && desc->ref_count > 0 && --desc->ref_count == 0)
    {
        free_phys_page(pa);
    }

    uint32_t pte_index = get_pte_index(va); 
    table->entries[pte_index] = NULL;
    invlpg((void*)va);

    return true;
}

static void handle_unmap_range_entry(uintptr_t va,
                                  page_table_t* table,
                                  void** run_start,
                                  uintptr_t* run_end)
{
    uint32_t entry = get_page_entry((void*)va);
    if (!(entry & PAGE_ENTRY_FLAG_PRESENT)) 
    {
        return; 
    }

    void* pa_ptr = (void*)(entry & PAGE_MASK);

    phys_page_descriptor_t* desc = phys_to_pfn(pa_ptr);
    bool should_free = desc && desc->ref_count > 0 && --desc->ref_count == 0;

    uint32_t pte_index = get_pte_index((void*)va);
    table->entries[pte_index] = 0;
    invlpg((void*)va);

    if (!should_free)
        return;

    uintptr_t pa = (uintptr_t)pa_ptr;

    if (*run_start && pa == *run_end) 
    {
        *run_end += PAGE_SIZE;
    }
    else 
    {
        if (*run_start) 
        {
            uintptr_t count = (*run_end - (uintptr_t)*run_start) / PAGE_SIZE;
            free_phys_pages((phys_alloc_t){ .addr = *run_start, .count = count });
        }

        *run_start = pa_ptr;
        *run_end   = pa + PAGE_SIZE;
    }
}

bool unmap_pages(void* va_ptr, uintptr_t count)
{
    uintptr_t va = (uintptr_t)va_ptr;

    void*    run_start = NULL;
    uintptr_t run_end  = 0;

    for (uintptr_t i = 0; i < count; i++, va += PAGE_SIZE) 
    {
        
        uint32_t pde = get_table_entry((void*)va);
        if (!(pde & PAGE_ENTRY_FLAG_PRESENT)) 
        {
            continue;
        }

        page_table_t* table = (page_table_t*)(pde & PAGE_MASK);
        handle_unmap_range_entry(va, table, &run_start, &run_end);
    }

    if (run_start) 
    {
        uintptr_t count = (run_end - (uintptr_t)run_start) / PAGE_SIZE;
        free_phys_pages((phys_alloc_t){ .addr = run_start, .count = count });
    }

    return true;
}
