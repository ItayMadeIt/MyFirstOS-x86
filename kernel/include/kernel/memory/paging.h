#ifndef __MEM_PAGING_H__
#define __MEM_PAGING_H__

#include <core/defs.h>
#include <kernel/core/paging.h>
#include <memory/phys_alloc/phys_alloc.h>

bool map_phys_pages(void* pa, void* va, usize_ptr count, u16 hw_flags);
bool map_phys_page (void* pa, void* va, u16 hw_flags);
void* identity_map_pages(void* pa_ptr, usize_ptr count, u16 hw_flags);

bool unmap_page(void* va);
bool unmap_pages(void* va, usize_ptr count);

void* virt_to_phys(void* va);

typedef enum virt_phys_flags
{
    VIRT_PHYS_FLAG_PRESENT  = 1 << 0,
    VIRT_PHYS_FLAG_DIRTY    = 1 << 1,
    VIRT_PHYS_FLAG_ACCESS   = 1 << 2,
    VIRT_PHYS_FLAG_USER     = 1 << 3,
    VIRT_PHYS_FLAG_WRITE    = 1 << 4,
    VIRT_PHYS_FLAG_NOEXEC   = 1 << 5,

} virt_phys_flags_t;

u32 get_phys_flags(void* va);
void clear_phys_flags(void* va, u32 flags);

#endif // __MEM_PAGING_H__