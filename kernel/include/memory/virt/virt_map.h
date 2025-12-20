#ifndef __VIRT_HANDLER_H__
#define __VIRT_HANDLER_H__

#include "core/num_defs.h"
#include "memory/core/pfn_desc.h"
#include "memory/virt/virt_region.h"

enum vflags
{
    VFLAG_READ   = 1 << 0,
    VFLAG_WRITE  = 1 << 1,
    VFLAG_EXEC   = 1 << 2,
    VFLAG_USER   = 1 << 3,
    VFLAG_SHARED = 1 << 4,
    VFLAG_COW    = 1 << 5,
};

void vmap_page(
    void* va,
    struct page* pfn, 
    enum virt_region_type vregion
);

void vmap_pages(
    void* va,
    struct page* pfn, 
    enum virt_region_type vregion,
    usize_ptr pages
);

void vclone(
    void* dst_va,
    void* src_va,
    usize_ptr count
);

void vshare_map(
    void* dst_va,
    void* src_va,
    usize_ptr count,
    u16 dst_paging_flags
);

void* vmap_identity(
    void* pa, 
    usize_ptr count, 
    enum virt_region_type type
);

void vunmap_pages(
    void* va, 
    usize_ptr count
);

void vunmap_page (
    void* va
);
#endif // __VIRT_HANDLER_H__