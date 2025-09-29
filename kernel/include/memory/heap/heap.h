#ifndef __HEAP_H__
#define __HEAP_H__

#include <core/defs.h>
#include <kernel/core/paging.h>
#include <memory/heap/heap_structs.h>

#define INVALID_MEMORY ~0

void* kalloc(uintptr_t size);
void* kalloc_aligned(uintptr_t alignment, uintptr_t size);
void* kalloc_pages(uintptr_t pages);
void* krealloc(void* addr, uintptr_t new_size);
void kfree(void* addr);

heap_slab_cache_t* kcreate_slab_cache(uintptr_t obj_size, const char* slab_name);
void* kalloc_cache(heap_slab_cache_t* cache);
void kfree_slab_cache(heap_slab_cache_t* slab_cache);

void init_heap(void* heap_addr, uintptr_t max_size, uintptr_t init_size);

#endif // __HEAP_H__