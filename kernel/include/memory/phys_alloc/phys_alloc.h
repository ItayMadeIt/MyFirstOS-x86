#ifndef __PHYS_ALLOC_H__
#define __PHYS_ALLOC_H__

#include <stddef.h>
#include <stdint.h>

extern uintptr_t kernel_size;
extern uintptr_t max_memory;
enum phys_page_type;

typedef struct phys_alloc
{
    void*     addr;
    uintptr_t count;
} phys_alloc_t;

typedef struct phys_run_vec
{
    uintptr_t    run_count;
    uintptr_t    total_pages;
    phys_alloc_t* runs;
} phys_run_vec_t;


extern void* (*alloc_phys_page)();
extern phys_alloc_t (*alloc_phys_pages)(const uintptr_t count);
extern void (*free_phys_page)(void* pa);
extern void (*free_phys_pages)(phys_alloc_t free_params);

#endif // __PHYS_ALLOC_H__