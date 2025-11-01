#ifndef __PHYS_ALLOC_H__
#define __PHYS_ALLOC_H__

#include "core/num_defs.h"
#include <stddef.h>
#include "core/num_defs.h"

extern usize_ptr kernel_size;
extern usize_ptr max_memory;
enum phys_page_type;

typedef struct phys_alloc
{
    void*     addr;
    usize_ptr count;
} phys_alloc_t;

typedef struct phys_run_vec
{
    usize_ptr    run_count;
    usize_ptr    total_pages;
    phys_alloc_t* runs;
} phys_run_vec_t;


extern void* (*alloc_phys_page)();
extern phys_alloc_t (*alloc_phys_pages)(const usize_ptr count);
extern void (*free_phys_page)(void* pa);
extern void (*free_phys_pages)(phys_alloc_t free_params);

#endif // __PHYS_ALLOC_H__