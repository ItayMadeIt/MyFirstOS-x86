#ifndef __VIRT_REGION_H__
#define __VIRT_REGION_H__

#include "core/num_defs.h"
#include "memory/core/pfn_desc.h"
#include <stdbool.h>
#include <kernel/memory/paging.h>

enum virt_region_type
{
    VREGION_UNKNOWN       = 0,
    VREGION_KERNELIMG     ,
    VREGION_PFN           ,
    VREGION_HEAP          ,
    VREGION_USER          ,
    VREGION_RESREVED      ,
    VREGION_CACHE         ,
    VREGION_MMIO          ,
    VREGION_ACPI          ,
    VREGION_DRIVER        ,
    VREGION_BIO_BUFFER    ,
};

const char* vregion_to_str(enum virt_region_type region);

void  kvregion_mark  (void* va, usize_ptr count, enum virt_region_type vregion, const char* name);
void* kvregion_reserve(usize_ptr count, enum virt_region_type vregion, const char* name);
void  kvregion_release(void* va);
bool  kvregion_is_free(void* va);
usize_ptr  kvregion_count(void* va);

void init_virt_region();

#endif // __VIRT_REGION_H__