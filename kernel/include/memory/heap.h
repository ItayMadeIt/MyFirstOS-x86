#ifndef __HEAP_H__
#define __HEAP_H__

#include <core/defs.h>
#include <core/paging.h>
#include <memory/heap_structs.h>

#define INVALID_MEMORY ~0

extern void* (*kalloc)(uint32_t size);
extern void* (*krealloc)(uint32_t new_size);
extern void  (*kfree)(void* ptr);

void setup_heap(void* heap_addr, uint32_t init_size);

#endif // __HEAP_H__