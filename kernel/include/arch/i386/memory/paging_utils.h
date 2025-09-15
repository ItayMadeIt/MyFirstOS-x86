#ifndef __RAW_VIRT_ALLOC_H__
#define __RAW_VIRT_ALLOC_H__

#include <core/defs.h>
#include <arch/i386/core/paging.h>

page_table_t* get_page_table(uint32_t index);
void* get_page_phys_base(page_table_t* table, uint32_t index);

uint32_t get_table_entry(void* virt_addr);
uint32_t get_page_entry(void* virt_addr);
void* get_phys_addr(void* virt_addr);

uint32_t get_table_entry(void* virt_addr);

uint32_t get_page_entry(void* virt_addr);

bool map_table_entry(void* phys_addr, void* virt_addr, uint16_t flags);

bool map_page_entry(void* phys_addr, void* virt_addr, uint16_t flags);

uint16_t pfn_flags_to_hw_flags(uint16_t flags);

static inline void invlpg(void* va) 
{
    asm volatile("invlpg (%0)" :: "r"(va) : "memory");
}
static inline void flush_tlb()
{
    uintptr_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %0, %%cr3" :: "r"(cr3) : "memory");
}
#endif // __RAW_VIRT_ALLOC_H__