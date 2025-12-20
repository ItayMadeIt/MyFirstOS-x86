#ifndef __HEAP_H__
#define __HEAP_H__

#include <core/defs.h>
#include <kernel/core/paging.h>
#include <memory/heap/heap_structs.h>

#define INVALID_MEMORY ~0

void* kmalloc(usize_ptr size);
void* kmalloc_aligned(usize_ptr alignment, usize_ptr size);
void* krealloc(void* addr, usize_ptr new_size);
void  kfree(void* addr);

heap_slab_cache_t* kcreate_slab_cache(usize_ptr obj_size, const char* slab_name);
void* kalloc_cache(heap_slab_cache_t* cache);
void kfree_slab_cache(heap_slab_cache_t* slab_cache);

void init_heap(void* heap_addr, usize_ptr max_size, usize_ptr init_size);

#endif // __HEAP_H__