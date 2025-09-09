#ifndef __MEMORY_MANAGER_H__
#define __MEMORY_MANAGER_H__

#include <memory/core/pfn_desc.h>
#include <core/defs.h>

extern void* (*alloc_phys_page)(enum phys_page_type type, uint16_t flags);
extern void (*free_phys_page)(void* pa);
void init_memory(boot_data_t* mbd);

#endif // __MEMORY_MANAGER_H__