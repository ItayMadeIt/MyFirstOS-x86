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
#define PAGE_SIZE 4096

typedef struct page_table
{
    page_entry entries[ENTRIES_AMOUNT];
}  __attribute__((aligned(ENTRIES_AMOUNT * sizeof(page_entry)))) page_table_t;

typedef struct page_directory
{
    page_table_entry entries[ENTRIES_AMOUNT];
}  __attribute__((aligned(ENTRIES_AMOUNT * sizeof(page_table_entry)))) page_directory_t;


void map_pages(
    page_directory_t* dir,
    page_table_t* table, 
    uint32_t virt_addr, 
    uint32_t phys_addr, 
    uint32_t pages, 
    uint16_t flags /*Assumes flags is valid*/);

void setup_paging();