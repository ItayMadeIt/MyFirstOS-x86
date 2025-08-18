#ifndef __HEAP_H__
#define __HEAP_H__

#include <core/defs.h>
#include <core/paging.h>
#include <memory/heap_structs.h>

#define SLAB_MIN_SIZE 16

#define SLOTS_PER_PAGE (PAGE_SIZE / SLAB_MIN_SIZE)

#define INVALID_MEMORY ~0

extern void* (*kmalloc)(uint32_t size);
extern void* (*krealloc)(uint32_t new_size);
extern void  (*kfree)(void* ptr);

void setup_heap(void* heap_addr, uint32_t init_size);

#endif // __HEAP_H__