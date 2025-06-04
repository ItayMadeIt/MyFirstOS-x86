#ifndef __PAGING_H__
#define __PAGING_H__

#define PAGE_ENTRY_FLAG(x) (1 << x)

#define PAGE_ENTRY_FLAG_PRESENT   PAGE_ENTRY_FLAG(0)
#define PAGE_ENTRY_FLAG_WRITE     PAGE_ENTRY_FLAG(1)
#define PAGE_ENTRY_FLAG_USER      PAGE_ENTRY_FLAG(2)
#define PAGE_ENTRY_FLAG_PWD       PAGE_ENTRY_FLAG(3)
#define PAGE_ENTRY_FLAG_PCD       PAGE_ENTRY_FLAG(4)
#define PAGE_ENTRY_FLAG_ACCESS    PAGE_ENTRY_FLAG(5)
#define PAGE_ENTRY_FLAG_PAGE_SIZE PAGE_ENTRY_FLAG(7)

#define PAGE_ENTRY_WRITE_KERNEL_FLAGS (PAGE_ENTRY_FLAG_PRESENT | PAGE_ENTRY_FLAG_WRITE)

#define INVALID_PAGE_MEMORY ~0
typedef uint32_t page_entry;
typedef uint32_t page_table_entry;

#define ENTRIES_AMOUNT 1024
#define PAGE_SIZE STOR_4Kib

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

#endif // __PAGING_H__