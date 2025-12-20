#ifndef __FRAME_ALLOC_H__
#define __FRAME_ALLOC_H__

#include <core/defs.h>
#include <memory/core/pfn_desc.h>
#include <kernel/boot/boot_data.h>
#include <stddef.h>
#include "core/num_defs.h"

page_t* frame_alloc_phys_pages(usize_ptr count);
void frame_free_phys_pages(page_t* pfn, usize_ptr count);

usize_ptr init_frame_allocator(boot_data_t* boot_data);
#endif // __FRAME_ALLOC_H__