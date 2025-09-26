#ifndef __PFN_ALLOC_H__
#define __PFN_ALLOC_H__

#include <core/defs.h>
#include <memory/core/pfn_desc.h>
#include <kernel/boot/boot_data.h>
#include <memory/phys_alloc/phys_alloc.h>

void* alloc_phys_page_pfn (enum phys_page_type type, uint16_t flags);
phys_alloc_t alloc_phys_pages_pfn(uintptr_t count, enum phys_page_type type, uint16_t flags);

void free_phys_page_pfn(void* pa);
void free_phys_pages_pfn(phys_alloc_t free_params);

void init_pfn_allocator(boot_data_t* boot_data);
#endif // __PFN_ALLOC_H__