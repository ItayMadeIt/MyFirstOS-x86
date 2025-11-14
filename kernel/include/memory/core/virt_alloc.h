#ifndef __VIRT_ALLOC_H__
#define __VIRT_ALLOC_H__

#include "core/num_defs.h"
#include "memory/core/pfn_desc.h"
#include "core/num_defs.h"
#include <stdbool.h>
#include <kernel/memory/paging.h>

enum virt_region_type
{
    VREGION_UNKNOWN       = PAGETYPE_NONE,
    VREGION_KERNEL        = PAGETYPE_KERNEL,
    VREGION_PFN           = PAGETYPE_PFN,
    VREGION_HEAP          = PAGETYPE_HEAP,
    VREGION_USER          = PAGETYPE_USER,
    VREGION_RESREVED      = PAGETYPE_RESERVED,
    VREGION_CACHE         = PAGETYPE_DISK_CACHE,
    VREGION_CACHE_CLONE   = PAGETYPE_DISK_CACHE_CLONE,
    VREGION_MMIO          = PAGETYPE_MMIO,
    VREGION_ACPI          = PAGETYPE_ACPI,
    VREGION_DRIVER        = PAGETYPE_DRIVER,
};
static inline enum phys_page_type virt_to_phys_type(enum virt_region_type v)
{
    switch (v) 
    {
        case VREGION_KERNEL:    return PAGETYPE_KERNEL;
        case VREGION_HEAP:      return PAGETYPE_HEAP;
        case VREGION_USER:      return PAGETYPE_USER;
        case VREGION_CACHE:     return PAGETYPE_DISK_CACHE;
        case VREGION_RESREVED:  return PAGETYPE_RESERVED;
        case VREGION_MMIO:      return PAGETYPE_MMIO;
        case VREGION_ACPI:      return PAGETYPE_ACPI;
        case VREGION_DRIVER:    return PAGETYPE_DRIVER;
        default:                return PAGETYPE_NONE;
    }
}

static inline enum virt_region_type phys_to_virt_type(enum phys_page_type p)
{
    switch (p) 
    {
        case PAGETYPE_KERNEL:     return VREGION_KERNEL;
        case PAGETYPE_HEAP:       return VREGION_HEAP;
        case PAGETYPE_USER:       return VREGION_USER;
        case PAGETYPE_DISK_CACHE: return VREGION_CACHE;
        case PAGETYPE_MMIO:       return VREGION_MMIO;
        case PAGETYPE_ACPI:       return VREGION_ACPI;
        case PAGETYPE_DRIVER:     return VREGION_DRIVER;
        case PAGETYPE_RESERVED:   return VREGION_RESREVED;
        default:                  return VREGION_UNKNOWN;
    }
}

void* kvalloc_pages(usize_ptr count, enum virt_region_type vregion, u16 page_flags);
void  kvfree_pages(void* va_ptr);

void* kvmap_phys_vec(const phys_run_vec_t* run, enum virt_region_type vregion, u16 page_flags);
void  kvunmap_phys_vec(void* va, const phys_run_vec_t* run);

void* vmap_identity(void* pa, usize_ptr count, enum virt_region_type type, u16 flags);

void* kvreserve_pages(usize_ptr count, enum virt_region_type vregion, const char* name);
void kvunreserve_pages(void* va);

// need to do it: void* vmap(usize_ptr* phys_pages, u64 page_count, u16 flags);
// need to do it: void  vunmap(void* addr, u64 page_count);

void init_virt_alloc();
void kvmark_region(void* from, void* to, enum virt_region_type vregion, const char* name);
void kvunmark_region(void* from, void* to);

// (currently not implemented) 
// void* kvrealloc_pages(void* va_ptr, usize_ptr count, u16 page_type, u16 page_flags);

#endif // __VIRT_ALLOC_H__