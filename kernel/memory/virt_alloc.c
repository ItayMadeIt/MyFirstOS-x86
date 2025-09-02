#include "core/paging.h"
#include "memory/page_frame.h"
#include "memory/paging_utils.h"
#include <memory/memory_manager.h>
#include <memory/virt_alloc.h>
#include <string.h>

static bool can_map_pages(void* va_ptr, uint32_t count)
{
    uint32_t va = (uint32_t)va_ptr;

    while (count) 
    {
        uint32_t dir_index = va >> 22;
        uint32_t table_index = (va >> 12) & 0x3FF;
        uint32_t pages_in_table = ENTRIES_AMOUNT - table_index;

        // If table exists, check each entry
        uint32_t table_entry = get_table_entry((void*)va);
        if ((table_entry & PAGE_ENTRY_FLAG_PRESENT) == false) 
        {
            // Skip over whole table worth of pages
            uint32_t skip = (pages_in_table < count) ? pages_in_table : count;
            va += skip * PAGE_SIZE;
            count -= skip;
            continue;
        } 

        page_table_t* table = get_page_table(dir_index);
        while (pages_in_table && count) 
        {
            if ((uint32_t)table->entries[table_index] & PAGE_ENTRY_FLAG_PRESENT)
                return false;

            va += PAGE_SIZE;
            table_index++;
            pages_in_table--;
            count--;
        }
    }

    return true;
}

static bool commit_map_pages(void* va_ptr, uint32_t count, enum phys_page_type page_type, uint32_t page_flags)
{
    uint32_t va = (uint32_t)va_ptr;
    while (count) 
    {
        uint32_t dir_index = va >> 22;
        uint32_t table_index = (va >> 12) & 0x3FF;
        uint32_t pages_in_table = ENTRIES_AMOUNT - table_index;

        uint32_t table_entry = get_table_entry((void*)va);
        
        if ((table_entry & PAGE_ENTRY_FLAG_PRESENT) == false)
        {
            // Allocate a new table
            void* new_table_phys = alloc_phys_page(PAGETYPE_TABLE, PAGE_ENTRY_FLAG_ALLPERMS);
            if (new_table_phys == NULL) 
                return false;

            map_table_entry(new_table_phys, (void*)va, PAGE_ENTRY_FLAG_ALLPERMS);
            
            memset((void*)(0xFFC00000 + dir_index * PAGE_SIZE), 0, PAGE_SIZE);
        }

        while (pages_in_table && count)
        {
            map_page_entry(
                alloc_phys_page(page_type, page_flags), 
                (void*)va, 
                pfn_flags_to_hw_flags(page_flags)
            );

            // Advance
            va += PAGE_SIZE;

            table_index++;
            pages_in_table--;
            count--;
        }
    }

    return true;
}

static bool force_map_phys_pages(void* pa_ptr, void* va_ptr, uint32_t count,
                                  enum phys_page_type page_type, uint32_t page_flags)
{
    uint32_t va = (uint32_t)va_ptr;
    uint32_t pa = (uint32_t)pa_ptr;

    while (count) 
    {
        uint32_t dir_index = va >> 22;
        uint32_t table_index = (va >> 12) & 0x3FF;
        uint32_t pages_in_table = ENTRIES_AMOUNT - table_index;

        uint32_t table_entry = get_table_entry((void*)va);
        if ((table_entry & PAGE_ENTRY_FLAG_PRESENT) == 0) 
        {
            void* new_table_phys = 
                alloc_phys_page(PAGETYPE_TABLE, PAGEFLAG_KERNEL);
            if (!new_table_phys) return false;

            map_table_entry(
                new_table_phys, 
                (void*)va, 
                pfn_flags_to_hw_flags(page_flags)
            );
            memset((void*)(0xFFC00000 + dir_index * PAGE_SIZE), 0, PAGE_SIZE);
        }

        while (pages_in_table && count) 
        {
            map_page_entry(
                (void*)pa, (void*)va, 
                pfn_flags_to_hw_flags(page_flags)
            );

            // Update PFN metadata 
            phys_page_descriptor_t* desc = phys_to_pfn((void*)pa);
            desc->type = page_type;
            desc->flags = page_flags;
            desc->ref_count++;

            va += PAGE_SIZE;
            pa += PAGE_SIZE;
            table_index++;
            pages_in_table--;
            count--;
        }
    }
    return true;
}


bool map_pages(void* va_ptr, uint32_t count, enum phys_page_type page_type, uint32_t page_flags)
{
    assert(((uintptr_t)va_ptr % PAGE_SIZE) == 0);
    assert(count > 0);
    assert(alloc_phys_page);

    if (!can_map_pages(va_ptr, count))
    {
        return false;
    }

    return commit_map_pages(va_ptr, count, page_type, page_flags);
}

void* identity_map_pages(void* pa_ptr, uint32_t count, enum phys_page_type page_type, uint32_t page_flags)
{
    assert(((uintptr_t)pa_ptr % PAGE_SIZE) == 0);
    assert(count > 0);
    assert(alloc_phys_page);

    void* va_ptr = pa_ptr;

    if (!can_map_pages(va_ptr, count))
    {
        return NULL;
    }

    if (force_map_phys_pages(pa_ptr, va_ptr, count, page_type, page_flags) == false)
    {
        return NULL;
    }

    return va_ptr;
}
