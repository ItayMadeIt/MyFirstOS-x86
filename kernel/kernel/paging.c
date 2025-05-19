#include <stdint.h>
#include <stdbool.h>

typedef struct virtual_metadata
{
    uint32_t virtual_addr;  // 0xC0000000
    uint32_t virtual_dir_index; // 768 -> 0xC0000000
    uint32_t virtual_table_index; // 15 -> 0xC0015000
    uint32_t physical_addr; // 0x00100032
    uint32_t pages_amount;  // 2048 -> 4096*2048 bytes
    uint16_t tail_12_bits;
} virtual_metadata_t;

typedef uint32_t page_entry;
typedef uint32_t page_table_entry;

#define ENTRIES_AMOUNT 1024

typedef struct page_table
{
    page_entry entries[ENTRIES_AMOUNT];
}  __attribute__((aligned(ENTRIES_AMOUNT * sizeof(page_entry)))) page_table_t;

typedef struct page_directory
{
    page_table_entry entries[ENTRIES_AMOUNT];
}  __attribute__((aligned(ENTRIES_AMOUNT * sizeof(page_table_entry)))) page_directory_t;

#define KERNEL_TABLES 2
#define BOOT_TABLES 1

page_directory_t kernel_directory; 
page_table_t kernel_tables[KERNEL_TABLES];
page_table_t boot_tables[KERNEL_TABLES];


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
    page_directory_t* dir,
    page_table_t* table, 
    uint32_t virt_addr, 
    uint32_t phys_addr, 
    uint32_t pages)
{
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

static void zero_page_directory(page_directory_t* page_dir)
{
    for (uint32_t i = 0; i < ENTRIES_AMOUNT; ++i) 
    {
        page_dir->entries[i] = 0;
    }
}

static void zero_page_table(page_table_t* page_table, uint32_t amount)
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

    map_pages(&kernel_directory, kernel_tables, 0xC0000000, 0x00100000, 2048);
    map_pages(&kernel_directory, boot_tables, 0x00000000, 0x00000000, 1024);

    set_page_directory(&kernel_directory);

    enable_paging();
}
