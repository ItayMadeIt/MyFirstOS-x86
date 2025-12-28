#include "memory/virt/virt_map.h"
#include "arch/i386/memory/paging_utils.h"
#include "kernel/memory/paging.h"
#include "core/num_defs.h"
#include "memory/core/pfn_desc.h"
#include "memory/virt/virt_region.h"

static u16 vregion_to_vflags(enum virt_region_type region)
{
    switch (region)
    {
        case VREGION_KERNELIMG:
            return VFLAG_READ | VFLAG_EXEC | VFLAG_WRITE;

        case VREGION_PFN:
            return VFLAG_READ | VFLAG_WRITE;

        case VREGION_HEAP:
            return VFLAG_READ | VFLAG_WRITE;

        case VREGION_USER:
            return VFLAG_READ | VFLAG_WRITE | VFLAG_USER;

        case VREGION_CACHE:
            return VFLAG_READ | VFLAG_SHARED;

        case VREGION_MMIO:
            return VFLAG_READ | VFLAG_WRITE | VFLAG_SHARED;

        case VREGION_ACPI:
            return VFLAG_READ | VFLAG_SHARED;

        case VREGION_BIO_BUFFER:
            return VFLAG_READ | VFLAG_WRITE;

        case VREGION_DRIVER:
            return VFLAG_READ | VFLAG_WRITE | VFLAG_SHARED;

        case VREGION_RESREVED:
            return 0;

        case VREGION_UNKNOWN:
        default:
            return 0;
    }
}

static u16  vflags_to_paging(u16 vmm_flags)
{
    u16 paging_flags = PAGING_FLAG_PRESENT;

    if (vmm_flags & VFLAG_READ)
    {
        paging_flags |= PAGING_FLAG_READ;
    }
    if (! (vmm_flags & VFLAG_EXEC) )
    {
        paging_flags |= PAGING_FLAG_NOEXEC; 
    }
    if (vmm_flags & VFLAG_USER)
    {
        paging_flags |= PAGING_FLAG_USER;
    }
    if (vmm_flags & VFLAG_WRITE)
    {
        paging_flags |= PAGING_FLAG_WRITE;
    }

    return paging_flags;
}

void vmap_pages(
    void* va,
    struct page* pfn,
    enum virt_region_type vregion,
    usize_ptr count)
{
    kvregion_mark(
        va, 
        count, 
        vregion, 
        vregion_to_str(vregion)
    );

    paging_map_pages(
        pfn_to_pa(pfn), 
        va, 
        count, 
        vflags_to_paging( vregion_to_vflags(vregion) )
    );
}

void vmap_page (
    void* va,
    struct page* pfn, 
    enum virt_region_type vregion)
{
    vmap_pages(va, pfn, vregion, 1);
}


void* vmap_identity(void* pa, usize_ptr count, enum virt_region_type type)
{
    vmap_pages(pa, pa, type, count);

    return pa;
}