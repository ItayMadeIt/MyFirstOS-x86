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
} page_table_t;

typedef struct page_directory
{
    page_table_entry entries[ENTRIES_AMOUNT];
} page_directory_t;




void debug_print(uint32_t value)
{
    volatile static char* vga = (volatile char*)0xB8000;
    vga+=sizeof(uint32_t) * 4 - 1;

    uint8_t it = 8;

    while (it)
    {
        char ch = (value & 0xF);
        if (ch >= 10)
        {
            ch += 'A' - 10;
        }
        else 
        {
            ch += '0';
        }

        *vga-- = 0x0F;
        *vga-- = ch;

        value >>= 4;
        --it;
    }
    vga += 80 * 2 + 1;
}

void dump_table(page_table_t* table, uint32_t start)
{
    debug_print((uint32_t)&table->entries[0]);
    debug_print(~(uint32_t)(0));
    debug_print((uint32_t)table->entries[start]);
    debug_print((uint32_t)table->entries[start+1]);
    debug_print((uint32_t)table->entries[start+2]);
    debug_print((uint32_t)table->entries[start+3]);
    debug_print(~(uint32_t)(0));
    debug_print(~(uint32_t)(0));
}

void dump_end_table(page_table_t* table, uint32_t entries_amount)
{
    debug_print((uint32_t)&table->entries[0]);
    debug_print(~(uint32_t)(0));
    debug_print((uint32_t)table->entries[entries_amount-4]);
    debug_print((uint32_t)table->entries[entries_amount-3]);
    debug_print((uint32_t)table->entries[entries_amount-2]);
    debug_print((uint32_t)table->entries[entries_amount-1]);
    debug_print(~(uint32_t)(0));
    debug_print(~(uint32_t)(0));
}
void dump_directory(page_directory_t* dir)
{
    debug_print(~1);
    debug_print((uint32_t)&dir->entries[768]);
    debug_print(dir->entries[768]);
    debug_print(~1);
}








void fetch_virtual_metadata(
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

bool map_virtual_metadata(
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
            page_directory_ptr->entries[dir_entry] = ((uint32_t)page_tables_ptr) | 0x3;
        }

        // Map the pages in this table
        for (; table_index < ENTRIES_AMOUNT && remaining_pages > 0; ++table_index)
        {
            page_tables_ptr->entries[table_index] = phys_addr | 0x3;
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

void map_pages(
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
