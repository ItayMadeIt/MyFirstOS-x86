#include <stdint.h>
#include <stdbool.h>
#include <core/paging.h>
#include <core/defs.h>

#define KERNEL_TABLES 2
#define BOOT_TABLES 1

page_directory_t kernel_directory; 
page_table_t kernel_tables[KERNEL_TABLES];
page_table_t boot_tables[KERNEL_TABLES];

void map_pages(
    page_directory_t* dir,
    page_table_t* table, 
    void* virt_addr_ptr, 
    void* phys_addr_ptr, 
    uint32_t pages, 
    uint16_t flags)
{
    uint32_t virt_addr = (uint32_t)virt_addr_ptr;
    uint32_t phys_addr = (uint32_t)phys_addr_ptr;

    uint32_t dir_entry = virt_addr >> 22;
    uint32_t table_index = (virt_addr >> 12) & 0x3FF;

    while (pages > 0)
    {
        // Map the current page directory entry 
        if (dir->entries[dir_entry] == 0)
        {
            dir->entries[dir_entry] = ((uint32_t)table) | flags;
        }

        // Map the pages in this table
        for (; table_index < ENTRIES_AMOUNT && pages > 0; ++table_index)
        {
            table->entries[table_index] = phys_addr | flags;
            phys_addr += 0x1000;
            --pages;
        }

        // Move to the next directory entry if needed
        if (pages > 0)
        {
            ++dir_entry;
            ++table;
            table_index = 0;
        }
    }
}

void zero_page_directory(page_directory_t* page_dir)
{
    for (uint32_t i = 0; i < ENTRIES_AMOUNT; ++i) 
    {
        page_dir->entries[i] = 0;
    }
}

void zero_page_table(page_table_t* page_table, uint32_t amount)
{
    do
    {
        for (uint32_t i = 0; i < ENTRIES_AMOUNT; ++i) 
        {
            page_table->entries[i] = 0;
        }

        --amount;
        ++page_table;

    } while(amount);
}


static inline void set_page_directory(page_directory_t* page_directory)
{
    asm volatile (
        "mov %0, %%cr3"
        :
        : "r" (page_directory)
    );
} 


static inline void enable_paging()
{
    asm volatile (
        "mov %%cr0, %%eax\n\t"
        "or $0x80000000, %%eax\n\t"
        "mov %%eax, %%cr0\n\t"
        :
        :
        : "eax"
    );
} 

void setup_paging()
{
    zero_page_directory(&kernel_directory);
    zero_page_table(kernel_tables, 2);
    zero_page_table(boot_tables, 1);

    map_pages(&kernel_directory, kernel_tables, (void*)0xC0000000, (void*)0x00100000, 2048, 0x3);
    map_pages(&kernel_directory, boot_tables, (void*)0x00000000, (void*)0x00000000, 1024, 0x3);

    map_pages(&kernel_directory, (page_table_t*)&kernel_directory, (void*)0xFFFFF000, &kernel_directory, 1, 0x3);

    set_page_directory(&kernel_directory);

    enable_paging();
}
