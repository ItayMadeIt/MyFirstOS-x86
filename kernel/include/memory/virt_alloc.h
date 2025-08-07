#ifndef __VIRT_ALLOC_H__
#define __VIRT_ALLOC_H__

#include <core/defs.h>

void* get_phys_addr(void* virt_addr);

uint32_t get_table_entry(void* virt_addr);

uint32_t get_page_entry(void* virt_addr);

bool map_table_entry(void* phys_addr, void* virt_addr, uint32_t flags);

bool map_page_entry(void* phys_addr, void* virt_addr, uint32_t flags);

void alloc_table(void* phys_table_addr, void* virt_addr, uint32_t flags);


#endif // __VIRT_ALLOC_H__