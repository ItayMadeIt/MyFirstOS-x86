#ifndef __HEAP_H__
#define __HEAP_H__

#include <core/defs.h>
#include <core/paging.h>
#include <memory/heap_structs.h>

#define INVALID_MEMORY ~0

void* kalloc(uint32_t size);
void* kalloc_pages(uint32_t pages);
void* krealloc(void* addr, uint32_t new_size);
void kfree(void* addr);

heap_slab_cache_t* kcreate_slab_cache(uint32_t obj_size, const char* slab_name);
void* kalloc_cache(heap_slab_cache_t* cache);
void kfree_slab_cache(heap_slab_cache_t* slab_cache);

void setup_heap(void* heap_addr, uint32_t init_size);

#endif // __HEAP_H__