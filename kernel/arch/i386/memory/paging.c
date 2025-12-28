#include "arch/i386/core/paging.h"
#include <kernel/memory/paging.h>
#include <kernel/core/paging.h>
#include <arch/i386/memory/paging_utils.h>
#include <memory/core/memory_manager.h>
#include <stdio.h>
#include "core/num_defs.h"


static page_table_t* ensure_page_table(u32 va)
{
    u32 dir_index = get_pde_index((void*)va);
    u32 entry = get_table_entry((void*)va);

    if (!(entry & PAGE_ENTRY_FLAG_PRESENT)) 
    {
        void* new_table_phys = mm_alloc_pagetable();
        assert(((u32)new_table_phys & (PAGE_SIZE-1)) == 0);

        if (!new_table_phys) 
        {
            return NULL;
        }
        
        map_table_entry(new_table_phys, (void*)va, PAGE_ENTRY_WRITE_KERNEL_FLAGS);
    }

    return get_page_table(dir_index);
}

static bool map_phys_range(void* pa_ptr, void* va_ptr, u32 count, u16 hw_flags)
{
    u32 va = (u32)va_ptr;
    u32 pa = (u32)pa_ptr;

    while (count) 
    {
        u32 table_index = get_pte_index((void*)va);
        u32 pages_in_table = ENTRIES_AMOUNT - table_index;

        page_table_t* table = ensure_page_table(va);
        if (!table)
        {
            return false;
        }

        while (pages_in_table && count) 
        {
            map_page_entry((void*)pa, (void*)va, hw_flags);

            va += PAGE_SIZE;
            pa += PAGE_SIZE;
            table_index++;
            pages_in_table--;
            count--;
        }
    }
    return true;
}

bool paging_map_pages(void* pa_ptr, void* va_ptr, u32 count, u16 paging_flags)
{
    u16 hw_flags = paging_to_hw_flags(paging_flags | PAGING_FLAG_PRESENT);

    assert(((u32)pa_ptr % PAGE_SIZE) == 0);
    assert(((u32)va_ptr % PAGE_SIZE) == 0);
    assert(count > 0);

    return map_phys_range(
        pa_ptr, va_ptr, 
        count, 
        hw_flags
    );
}

bool paging_map_page(void* pa_ptr, void* va_ptr, u16 paging_flags)
{
    return paging_map_pages(pa_ptr, va_ptr, 1, paging_flags);
}


bool paging_unmap_page(void* va)
{
    page_table_t* table = (page_table_t*)(get_table_entry(va) & PAGE_MASK);

    u32 entry = get_page_entry(va);
    if (!entry || !(entry & PAGE_ENTRY_FLAG_PRESENT)) 
        return false;

    u32 pte_index = get_pte_index(va); 
    table->entries[pte_index] = NULL;
    invlpg((void*)va);

    return true;
}

bool paging_unmap_pages(void* va_ptr, usize_ptr count)
{
    usize_ptr va = (usize_ptr)va_ptr;

    for (usize_ptr i = 0; i < count; i++, va += PAGE_SIZE) 
    {
        u32 pde = get_table_entry((void*)va);
        if (!(pde & PAGE_ENTRY_FLAG_PRESENT)) 
        {
            continue;
        }

        page_table_t* table = (page_table_t*)(pde & PAGE_MASK);
        u32 entry = get_page_entry((void*)va);
        if (!(entry & PAGE_ENTRY_FLAG_PRESENT)) 
        {
            continue;
        }

        u32 pte_index = get_pte_index((void*)va);
        table->entries[pte_index] = 0;

        invlpg((void*)va);
    }

    return true;
}
