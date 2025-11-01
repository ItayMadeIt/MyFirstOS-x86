#ifndef __I386_PAGING_H__
#define __I386_PAGING_H__

#include "core/num_defs.h"
#define PAGE_ENTRY_FLAG(x) (1 << x)

#define PAGE_ENTRY_FLAG_PRESENT   PAGE_ENTRY_FLAG(0)
#define PAGE_ENTRY_FLAG_WRITE     PAGE_ENTRY_FLAG(1)
#define PAGE_ENTRY_FLAG_USER      PAGE_ENTRY_FLAG(2)
#define PAGE_ENTRY_FLAG_PWD       PAGE_ENTRY_FLAG(3)
#define PAGE_ENTRY_FLAG_PCD       PAGE_ENTRY_FLAG(4)
#define PAGE_ENTRY_FLAG_ACCESS    PAGE_ENTRY_FLAG(5)
#define PAGE_ENTRY_FLAG_DIRTY     PAGE_ENTRY_FLAG(6)
#define PAGE_ENTRY_FLAG_PAGE_SIZE PAGE_ENTRY_FLAG(7)

#define PAGE_ENTRY_WRITE_KERNEL_FLAGS (PAGE_ENTRY_FLAG_PRESENT | PAGE_ENTRY_FLAG_WRITE)

#define INVALID_PAGE_MEMORY (u32)(~0)

#define ENTRIES_AMOUNT 1024
#define PAGE_SIZE STOR_4KiB
#define PAGE_SHIFT 12
#define PAGE_MASK (usize_ptr)(~(PAGE_SIZE-1))

typedef struct __attribute__((aligned(ENTRIES_AMOUNT * sizeof(u32)))) 
    page_table
{
    void* entries[ENTRIES_AMOUNT];
}  page_table_t;

typedef struct __attribute__((aligned(ENTRIES_AMOUNT * sizeof(u32)))) 
    page_directory
{
    void* entries[ENTRIES_AMOUNT];
}  page_directory_t;


extern page_directory_t page_directory; 

void init_paging();

#endif // __PAGING_H__