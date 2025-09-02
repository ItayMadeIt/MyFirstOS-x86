#ifndef __PAGING_H__
#define __PAGING_H__

#include <stdint.h>
#define PAGE_ENTRY_FLAG(x) (1 << x)

#define PAGE_ENTRY_FLAG_PRESENT   PAGE_ENTRY_FLAG(0)
#define PAGE_ENTRY_FLAG_WRITE     PAGE_ENTRY_FLAG(1)
#define PAGE_ENTRY_FLAG_USER      PAGE_ENTRY_FLAG(2)
#define PAGE_ENTRY_FLAG_PWD       PAGE_ENTRY_FLAG(3)
#define PAGE_ENTRY_FLAG_PCD       PAGE_ENTRY_FLAG(4)
#define PAGE_ENTRY_FLAG_ACCESS    PAGE_ENTRY_FLAG(5)
#define PAGE_ENTRY_FLAG_PAGE_SIZE PAGE_ENTRY_FLAG(7)

#define PAGE_ENTRY_WRITE_KERNEL_FLAGS (PAGE_ENTRY_FLAG_PRESENT | PAGE_ENTRY_FLAG_WRITE)
#define PAGE_ENTRY_FLAG_ALLPERMS (PAGE_ENTRY_FLAG_PRESENT | PAGE_ENTRY_FLAG_WRITE | PAGE_ENTRY_FLAG_USER)

#define INVALID_PAGE_MEMORY (uint32_t)(~0)

#define ENTRIES_AMOUNT 1024
#define PAGE_SIZE STOR_4KiB

typedef struct page_table
{
    void* entries[ENTRIES_AMOUNT];
}  __attribute__((aligned(ENTRIES_AMOUNT * sizeof(uint32_t)))) page_table_t;

typedef struct page_directory
{
    void* entries[ENTRIES_AMOUNT];
}  __attribute__((aligned(ENTRIES_AMOUNT * sizeof(uint32_t)))) page_directory_t;


#define round_page_up(x)   (((uint64_t)x + 0xFFFull) & ~0xFFFull)
#define round_page_down(x) ((uint64_t)x & ~0xFFFull)

extern page_directory_t page_directory; 

void setup_paging();

#endif // __PAGING_H__