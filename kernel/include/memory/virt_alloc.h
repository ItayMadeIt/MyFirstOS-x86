#ifndef __VIRT_ALLOC_H__
#define __VIRT_ALLOC_H__

#include <core/defs.h>

uint32_t get_phys_addr(uint32_t virt_addr);

uint32_t get_table_entry(uint32_t virt_addr);

uint32_t get_page_entry(uint32_t virt_addr);

bool map_table_entry(uint32_t phys_addr, uint32_t virt_addr, uint32_t flags);

bool map_page_entry(uint32_t phys_addr, uint32_t virt_addr, uint32_t flags);

void alloc_table(uint32_t phys_table_addr, uint32_t virt_addr, uint32_t flags);


#endif // __VIRT_ALLOC_H__