#include <kernel_loader/paging_defs.h>

static void fetch_virtual_metadata(
    virtual_metadata_t* virt_metadata, 
    uint32_t virtual_addr, 
    uint32_t physical_addr, 
    uint32_t pages_amount)
{
    virt_metadata->physical_addr = physical_addr & (~0xFFF);
    virt_metadata->virtual_addr = virtual_addr & (~0xFFF);

    virt_metadata->virtual_dir_index = virt_metadata->virtual_addr >> 22;
    virt_metadata->virtual_table_index = (virt_metadata->virtual_addr >> 12) & 0x3FF;

    virt_metadata->pages_amount = pages_amount;

    virt_metadata->tail_12_bits = physical_addr & 0xFFF;
}

static bool map_virtual_metadata(
    page_directory_t* page_directory_ptr,
    page_table_t* page_tables_ptr,
    const virtual_metadata_t* virt_metadata)
{
    uint32_t dir_entry = virt_metadata->virtual_dir_index;
    uint32_t table_index = virt_metadata->virtual_table_index;
    uint32_t phys_addr = virt_metadata->physical_addr;

    uint32_t remaining_pages = virt_metadata->pages_amount;

    while (remaining_pages > 0)
    {
        // Map the current page directory entry 
        if (page_directory_ptr->entries[dir_entry] == 0)
        {
            page_directory_ptr->entries[dir_entry] = ((uint32_t)page_tables_ptr) | 0x83;
        }

        // Map the pages in this table
        for (; table_index < ENTRIES_AMOUNT && remaining_pages > 0; ++table_index)
        {
            page_tables_ptr->entries[table_index] = phys_addr | 0x83;
            phys_addr += 0x1000;
            --remaining_pages;
        }

        // Move to the next directory entry if needed
        if (remaining_pages > 0)
        {
            ++dir_entry;
            ++page_tables_ptr;
            table_index = 0;
        }
    }

    return true;
}

static void map_pages(
    uint32_t dir_value,
    uint32_t table_value, 
    uint32_t virt_addr, 
    uint32_t phys_addr, 
    uint32_t pages)
{
    page_directory_t* dir = (page_directory_t*)dir_value;
    page_table_t* table = (page_table_t*)table_value;

    virtual_metadata_t metadata;
    fetch_virtual_metadata(
        &metadata,
        virt_addr,
        phys_addr, 
        pages
    );

    map_virtual_metadata(
        dir, 
        table, 
        &metadata
    ); 


}

static void zero_page_directory(uint32_t page_dir)
{
    volatile page_directory_t* page_dir_ptr = (page_directory_t*)page_dir;

    for (uint32_t i = 0; i < ENTRIES_AMOUNT; ++i) 
    {
        page_dir_ptr->entries[i] = 0;
    }
}

static void zero_page_table(uint32_t page_table, uint32_t amount)
{
    volatile page_table_t* page_table_ptr = (page_table_t*)page_table;

    do
    {
        for (uint32_t i = 0; i < ENTRIES_AMOUNT; ++i) 
        {
            page_table_ptr->entries[i] = 0;
        }

        --amount;
        ++page_table_ptr;

    } while(amount);
}


void setup_paging(
    uint32_t dir,
    uint32_t kernel_table,
    uint32_t boot_table)
{
    zero_page_directory(dir);
    zero_page_table(kernel_table, 2);
    zero_page_table(boot_table, 1);

    map_pages(dir, kernel_table, 0xC0000000, 0x00100000, 2048);
    
    map_pages(dir, boot_table, 0x00000000, 0x00000000, 1024);
}
