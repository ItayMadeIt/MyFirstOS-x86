#ifndef __MEM_PAGING_H__
#define __MEM_PAGING_H__

#include <core/defs.h>
#include <kernel/core/paging.h>

bool paging_map_pages(void* pa, void* va, usize_ptr count, u16 paging_flags);
bool paging_map_page (void* pa, void* va, u16 paging_flags);

bool paging_unmap_pages(void* va, usize_ptr count);
bool paging_unmap_page(void* va);

void* virt_to_phys(void* va);

typedef enum paging_flags
{
    PAGING_FLAG_PRESENT  = 1 << 0,
    PAGING_FLAG_DIRTY    = 1 << 1,
    PAGING_FLAG_ACCESS   = 1 << 2,
    PAGING_FLAG_USER     = 1 << 3,
    PAGING_FLAG_WRITE    = 1 << 4,
    PAGING_FLAG_NOEXEC   = 1 << 5,
    PAGING_FLAG_READ     = 1 << 6,

} paging_flags_t;

u16  paging_get_flags(void* va);
void paging_clear_flags(void* va, u16 paging_flags);
u16  paging_to_hw_flags(u16 paging_flags);

#endif // __MEM_PAGING_H__