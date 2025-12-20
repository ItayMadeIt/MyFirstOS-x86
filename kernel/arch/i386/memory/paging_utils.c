#include <arch/i386/core/paging.h>
#include <memory/phys_alloc/bitmap_alloc.h>
#include <memory/core/pfn_desc.h>
#include <memory/core/memory_manager.h>
#include <arch/i386/memory/paging_utils.h>
#include <string.h>
#include <kernel/memory/paging.h>


u32 get_pde_index(void *va)
{
    return (usize_ptr)va >> 22;
}

u32 get_pte_index(void *va)
{
    return ((usize_ptr)va >> 12) & 0x3FF;
}

static bool can_map_pages(void* va_ptr, u32 count)
{
    u32 va = (u32)va_ptr;

    while (count) 
    {
        u32 dir_index   = get_pde_index((void*)va);
        u32 table_index = get_pte_index((void*)va);
        u32 pages_in_table = ENTRIES_AMOUNT - table_index;

        page_table_t* table = get_page_table(dir_index);
        // no table = can be fully utilized
        if (!table)
        {
            u32 skip = (pages_in_table < count) ? pages_in_table : count;
            va    += skip * PAGE_SIZE;
            count -= skip;
            continue;
        }

        // Go over each page and check if it can be allocated
        while (pages_in_table && count) 
        {
            if ((u32)table->entries[table_index] & PAGE_ENTRY_FLAG_PRESENT)
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

bool map_table_entry(void* phys_addr_ptr, void* virt_addr_ptr, u16 flags)
{
    u32 phys_addr = (u32)phys_addr_ptr;

    // Calculate indices
    u32 page_dir_index = get_pde_index(virt_addr_ptr);

    page_directory_t* page_directory = (page_directory_t*)0xFFFFF000;
    if ((u32)page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT)
        return false;
    
    page_directory->entries[page_dir_index] = (void*)( (phys_addr & PAGE_MASK) | (u32)flags );
    
    memset((void*)(0xFFC00000 + page_dir_index*PAGE_SIZE), 0, PAGE_SIZE);

    return true;
}

bool map_page_entry(void* phys_addr_ptr, void* virt_addr_ptr, u16 flags)
{
    u32 phys_addr = (u32)phys_addr_ptr;

    // Calculate indices
    u32 page_dir_index = get_pde_index(virt_addr_ptr);
    u32 page_table_index = get_pte_index(virt_addr_ptr);

    // Get page directory, ensure it's valid
    page_directory_t* page_directory = (void*)0xFFFFF000;    
    if (((u32)page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return false;
    }

    // Get page table, ensure it's valid
    page_table_t* page_table = (page_table_t*)(0xFFC00000 + (page_dir_index * PAGE_SIZE));
    page_table->entries[page_table_index] = (void*)((phys_addr & 0xFFFFF000) | ((u32)flags & 0x00000FFF));

    invlpg(virt_addr_ptr);

    return true;
}

page_table_t *get_page_table(u32 dir_index)
{
    page_directory_t* page_dir = (page_directory_t*)0xFFFFF000;
    u32 page_dir_entry = (u32)page_dir->entries[dir_index];
    
    if ((page_dir_entry & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return NULL;
    }

    return (page_table_t*)(0xFFC00000 + dir_index * PAGE_SIZE);
}
void* get_page_phys_base(page_table_t *table, u32 index)
{
    if (((u32)table->entries[index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
        return NULL;

    return (void*) (
        ((u32)table->entries[index] & ~0xFFF) 
    );
}
void* virt_to_phys(void* virt_addr_ptr)
{
    u32 virt_addr = (u32)virt_addr_ptr;

    // Calculate indices
    u32 page_dir_index = (virt_addr) >> 22;
    u32 page_table_index = ((virt_addr) >> 12) & 0x3FF;

    // Get page directory, ensure it's valid
    page_directory_t* page_directory = (void*)0xFFFFF000;    
    if (((u32)page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return NULL;
    }

    // Get page table, ensure it's valid
    page_table_t* page_table = (page_table_t*)(0xFFC00000 + (page_dir_index * 0x1000));
    if (((u32)page_table->entries[page_table_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return NULL;
    }

    return (void*) (
        ((u32)page_table->entries[page_table_index] & ~0xFFF) | (virt_addr & 0xFFF) 
    );
}

void paging_map_identity(u32 pa, u32 count, u16 paging_flags)
{
    can_map_pages((void*)pa, count);

    paging_map_pages(
        (void*)pa, 
        (void*)pa, 
        count, paging_flags
    );
}

void set_page_entry(void *virt_addr, u32 entry) 
{
    u32 vaddr = (u32)virt_addr;

    u32 page_dir_index   = vaddr >> 22;
    u32 page_table_index = (vaddr >> 12) & 0x3FF;

    page_directory_t *page_directory = (void*)0xFFFFF000;

    if (((u32) page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT) == 0) 
    {
        return;
    }

    page_table_t *page_table = (page_table_t*)(0xFFC00000 + (page_dir_index * PAGE_SIZE));

    page_table->entries[page_table_index] = (void*)entry;

    asm volatile("invlpg (%0)" ::"r"(virt_addr) : "memory");
}

u16 paging_get_flags(void *va) 
{
    u32 result = 0;

    u32 entry = get_page_entry(va);
    if (entry & (PAGE_ENTRY_FLAG_PRESENT))
        result |= PAGING_FLAG_PRESENT;

    if (entry & PAGE_ENTRY_FLAG_DIRTY)
        result |= PAGING_FLAG_DIRTY;

    if (entry & PAGE_ENTRY_FLAG_ACCESS)
        result |= PAGING_FLAG_ACCESS;

    if (entry & PAGE_ENTRY_FLAG_WRITE)
        result |= PAGING_FLAG_WRITE;

    if (entry & PAGE_ENTRY_FLAG_USER)
        result |= PAGING_FLAG_USER;

    return result;
}

void paging_clear_flags(void *va, u16 flags) 
{
    u32 entry = get_page_entry(va);
    assert(! (flags & PAGING_FLAG_PRESENT));

    if (entry & PAGING_FLAG_ACCESS)
        entry &= ~PAGE_ENTRY_FLAG_ACCESS;

    if (entry & PAGING_FLAG_DIRTY)
        entry &= ~PAGE_ENTRY_FLAG_DIRTY;

    if (entry & PAGING_FLAG_WRITE)
        entry &= ~PAGE_ENTRY_FLAG_WRITE;

    if (entry & PAGING_FLAG_USER)
        entry &= ~PAGE_ENTRY_FLAG_USER;
    
    set_page_entry(va, entry);
}

u16 paging_to_hw_flags(u16 paging_flags)
{
    u16 hw_flags = 0;

    if (paging_flags & PAGING_FLAG_ACCESS)
        hw_flags |= PAGE_ENTRY_FLAG_ACCESS;

    if (paging_flags & PAGING_FLAG_DIRTY)
        hw_flags |= PAGE_ENTRY_FLAG_DIRTY;

    if (paging_flags & PAGING_FLAG_WRITE)
        hw_flags |= PAGE_ENTRY_FLAG_WRITE;

    if (paging_flags & PAGING_FLAG_USER)
        hw_flags |= PAGE_ENTRY_FLAG_USER;

    if (paging_flags & PAGING_FLAG_PRESENT)
        hw_flags |= PAGE_ENTRY_FLAG_PRESENT;

    return hw_flags;
}

u32 get_table_entry(void* virt_addr_ptr)
{
    u32 virt_addr = (u32)virt_addr_ptr;

    // Calculate index
    u32 page_dir_index = (virt_addr) >> 22;

    page_directory_t* page_directory = (void*)0xFFFFF000;  

    return (u32)page_directory->entries[page_dir_index];
}
u32 get_page_entry(void* virt_addr_ptr)
{
    u32 virt_addr = (u32)virt_addr_ptr;
    // Calculate indices
    u32 page_dir_index = (virt_addr) >> 22;
    u32 page_table_index = ((virt_addr) >> 12) & 0x3FF;

    // Get page directory, ensure it's valid
    page_directory_t* page_directory = (void*)0xFFFFF000;    
    if (((u32)page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return INVALID_PAGE_MEMORY;
    }

    // Get page table, ensure it's valid
    page_table_t* page_table = (page_table_t*)(0xFFC00000 + (page_dir_index * 0x1000));
    return (u32)page_table->entries[page_table_index];
}

u16 pfn_to_hw_flags(u16 pfn_flags)
{
    u16 result = PAGE_ENTRY_FLAG_PRESENT;
    if ((pfn_flags & PAGEFLAG_KERNEL) == false)
    {
        result |= PAGE_ENTRY_FLAG_USER;
    }
    if ((pfn_flags & PAGEFLAG_READONLY) == false)
    {
        result |= PAGE_ENTRY_FLAG_WRITE;
    }
    if ((pfn_flags & PAGEFLAG_NOEXEC) == false)
    {
        // No NX bit in 32 bit flags...
    }
    
    return result;
}