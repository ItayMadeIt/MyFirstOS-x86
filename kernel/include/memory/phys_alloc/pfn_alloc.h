#ifndef __PFN_ALLOC_H__
#define __PFN_ALLOC_H__

#include <core/defs.h>
#include <memory/core/pfn_desc.h>
#include <kernel/boot/boot_data.h>
#include <memory/phys_alloc/phys_alloc.h>
#include <stddef.h>
#include "core/num_defs.h"

void* pfn_alloc_phys_page ();
phys_alloc_t pfn_alloc_phys_pages(usize_ptr count);

void pfn_free_phys_page(void* pa);
void pfn_free_phys_pages(phys_alloc_t free_params);

phys_run_vec_t pfn_alloc_phys_run_vector(usize_ptr count);
void pfn_free_phys_pages_vector(phys_run_vec_t vector);

void init_pfn_allocator(boot_data_t* boot_data);
#endif // __PFN_ALLOC_H__