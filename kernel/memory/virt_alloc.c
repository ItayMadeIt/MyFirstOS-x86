#include <memory/phys_alloc.h>
#include <memory/virt_alloc.h>
#include <core/paging.h>

void* get_phys_addr(void* virt_addr_ptr)
{
    uint32_t virt_addr = (uint32_t)virt_addr_ptr;

    // Calculate indices
    uint32_t page_dir_index = (virt_addr) >> 22;
    uint32_t page_table_index = ((virt_addr) >> 12) & 0x3FF;

    // Get page directory, ensure it's valid
    page_directory_t* page_directory = (void*)0xFFFFF000;    
    if ((page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return (void*) INVALID_PAGE_MEMORY;
    }

    // Get page table, ensure it's valid
    page_table_t* page_table = (page_table_t*)(0xFFC00000 + (page_dir_index * 0x1000));
    if ((page_table->entries[page_table_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return (void*) INVALID_PAGE_MEMORY;
    }

    return (void*) (
        (page_table->entries[page_table_index] & ~0xFFF) | (virt_addr & 0xFFF) 
    );
}

uint32_t get_table_entry(void* virt_addr_ptr)
{
    uint32_t virt_addr = (uint32_t)virt_addr_ptr;

    // Calculate indices
    uint32_t page_dir_index = virt_addr >> 22;

    page_directory_t* page_directory = (page_directory_t*)0xFFFFF000;
    return page_directory->entries[page_dir_index];
}

uint32_t get_page_entry(void* virt_addr_ptr)
{
    uint32_t virt_addr = (uint32_t)virt_addr_ptr;
    // Calculate indices
    uint32_t page_dir_index = (virt_addr) >> 22;
    uint32_t page_table_index = ((virt_addr) >> 12) & 0x3FF;

    // Get page directory, ensure it's valid
    page_directory_t* page_directory = (void*)0xFFFFF000;    
    if ((page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return (uint32_t)NULL;
    }

    // Get page table, ensure it's valid
    page_table_t* page_table = (page_table_t*)(0xFFC00000 + (page_dir_index * 0x1000));
    return page_table->entries[page_table_index];
}

bool map_table_entry(void* phys_addr_ptr, void* virt_addr_ptr, uint32_t flags)
{
    uint32_t virt_addr = (uint32_t)virt_addr_ptr;
    uint32_t phys_addr = (uint32_t)phys_addr_ptr;

    // Calculate indices
    uint32_t page_dir_index = virt_addr >> 22;

    page_directory_t* page_directory = (page_directory_t*)0xFFFFF000;
    page_directory->entries[page_dir_index] = (phys_addr & ~0xFFF) | flags;

    return true;
}

bool map_page_entry(void* phys_addr_ptr, void* virt_addr_ptr, uint32_t flags)
{
    uint32_t phys_addr = (uint32_t)phys_addr_ptr;
    uint32_t virt_addr = (uint32_t)virt_addr_ptr;

    // Calculate indices
    uint32_t page_dir_index = (virt_addr) >> 22;
    uint32_t page_table_index = ((virt_addr) >> 12) & 0x3FF;

    // Get page directory, ensure it's valid
    page_directory_t* page_directory = (void*)0xFFFFF000;    
    if ((page_directory->entries[page_dir_index] & PAGE_ENTRY_FLAG_PRESENT) == 0)
    {
        return false;
    }

    // Get page table, ensure it's valid
    page_table_t* page_table = (page_table_t*)(0xFFC00000 + (page_dir_index * 0x1000));
    page_table->entries[page_table_index] = (phys_addr & 0xFFFFF000) | (flags & 0x00000FFF);

    return true;
}

void alloc_table(void* phys_table_addr_ptr, void* virt_addr_ptr, uint32_t flags)
{
    uint32_t virt_addr = (uint32_t)virt_addr_ptr;

    map_table_entry(phys_table_addr_ptr, virt_addr_ptr, flags);

    for (uint32_t i = 0; i < ENTRIES_AMOUNT; i++)
    {
        uint32_t cur_virt_addr = virt_addr + i * PAGE_SIZE;
        map_page_entry(alloc_phys_page(), (void*)cur_virt_addr, flags);
    }
}