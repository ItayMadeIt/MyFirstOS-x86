#include <arch/i386/core/paging.h>
#include <memory/phys_alloc/bitmap_alloc.h>
#include <memory/core/pfn_desc.h>
#include <memory/core/memory_manager.h>
#include <arch/i386/memory/paging_utils.h>
#include <string.h>

bool map_table_entry(void* phys_addr_ptr, void* virt_addr_ptr, uint16_t flags)
{
    uint32_t virt_addr = (uint32_t)virt_addr_ptr;
    uint32_t phys_addr = (uint32_t)phys_addr_ptr;

    // Calculate indices
    uint32_t page_dir_index = virt_addr >> 22;

    page_directory_t* page_directory = (page_directory_t*)0xFFFFF000;
    if ((uint32_t)page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT)
        return false;
    
    page_directory->entries[page_dir_index] = (void*)( (phys_addr & ~0xFFF) | (uint32_t)flags );
    
    memset((void*)(0xFFC00000 + page_dir_index*PAGE_SIZE), 0, PAGE_SIZE);

    return true;
}

bool map_page_entry(void* phys_addr_ptr, void* virt_addr_ptr, uint16_t flags)
{
    uint32_t phys_addr = (uint32_t)phys_addr_ptr;
    uint32_t virt_addr = (uint32_t)virt_addr_ptr;

    // Calculate indices
    uint32_t page_dir_index = (virt_addr) >> 22;
    uint32_t page_table_index = ((virt_addr) >> 12) & 0x3FF;

    // Get page directory, ensure it's valid
    page_directory_t* page_directory = (void*)0xFFFFF000;    
    if (((uint32_t)page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return false;
    }

    // Get page table, ensure it's valid
    page_table_t* page_table = (page_table_t*)(0xFFC00000 + (page_dir_index * 0x1000));
    page_table->entries[page_table_index] = (void*)((phys_addr & 0xFFFFF000) | ((uint32_t)flags & 0x00000FFF));

    return true;
}


page_table_t* get_page_table(uint32_t dir_index)
{
    page_directory_t* page_dir = (page_directory_t*)0xFFFFF000;
    uint32_t page_dir_entry = (uint32_t)page_dir->entries[dir_index];
    
    if ((page_dir_entry & PAGE_ENTRY_FLAG_PRESENT) == 0)
        return NULL;

    return (page_table_t*)(0xFFC00000 + dir_index * PAGE_SIZE);
}
void* get_page_phys_base(page_table_t *table, uint32_t index)
{
    if (((uint32_t)table->entries[index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
        return NULL;

    return (void*) (
        ((uint32_t)table->entries[index] & ~0xFFF) 
    );
}
void* get_phys_addr(void* virt_addr_ptr)
{
    uint32_t virt_addr = (uint32_t)virt_addr_ptr;

    // Calculate indices
    uint32_t page_dir_index = (virt_addr) >> 22;
    uint32_t page_table_index = ((virt_addr) >> 12) & 0x3FF;

    // Get page directory, ensure it's valid
    page_directory_t* page_directory = (void*)0xFFFFF000;    
    if (((uint32_t)page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return NULL;
    }

    // Get page table, ensure it's valid
    page_table_t* page_table = (page_table_t*)(0xFFC00000 + (page_dir_index * 0x1000));
    if (((uint32_t)page_table->entries[page_table_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return NULL;
    }

    return (void*) (
        ((uint32_t)page_table->entries[page_table_index] & ~0xFFF) | (virt_addr & 0xFFF) 
    );
}
uint32_t get_table_entry(void* virt_addr_ptr)
{
    uint32_t virt_addr = (uint32_t)virt_addr_ptr;

    // Calculate index
    uint32_t page_dir_index = (virt_addr) >> 22;

    page_directory_t* page_directory = (void*)0xFFFFF000;  

    return (uint32_t)page_directory->entries[page_dir_index];
}
uint32_t get_page_entry(void* virt_addr_ptr)
{
    uint32_t virt_addr = (uint32_t)virt_addr_ptr;
    // Calculate indices
    uint32_t page_dir_index = (virt_addr) >> 22;
    uint32_t page_table_index = ((virt_addr) >> 12) & 0x3FF;

    // Get page directory, ensure it's valid
    page_directory_t* page_directory = (void*)0xFFFFF000;    
    if (((uint32_t)page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return INVALID_PAGE_MEMORY;
    }

    // Get page table, ensure it's valid
    page_table_t* page_table = (page_table_t*)(0xFFC00000 + (page_dir_index * 0x1000));
    return (uint32_t)page_table->entries[page_table_index];
}

uint16_t pfn_flags_to_hw_flags(uint16_t flags)
{
    uint16_t result = PAGE_ENTRY_FLAG_PRESENT;
    if ((flags & PAGEFLAG_KERNEL) == false)
    {
        result |= PAGE_ENTRY_FLAG_USER;
    }
    if ((flags & PAGEFLAG_READONLY) == false)
    {
        result |= PAGE_ENTRY_FLAG_WRITE;
    }
    if ((flags & PAGEFLAG_NOEXEC) == false)
    {
        // No NX bit in 32 bit flags...
    }
    
    return result;
}