#ifndef __RAW_VIRT_ALLOC_H__
#define __RAW_VIRT_ALLOC_H__

#include <core/defs.h>
#include <arch/i386/core/paging.h>

u32 get_pde_index(void* va);
u32 get_pte_index(void* va);

page_table_t* get_page_table(u32 index);
void* get_page_phys_base(page_table_t* table, u32 index);

u32 get_table_entry(void* virt_addr);
u32 get_page_entry(void* virt_addr);

void* virt_to_phys(void* virt_addr);

void paging_map_identity(u32 pa, u32 count, u16 paging_flags);

void set_page_entry(void* virt_addr, u32 entry);

u32 get_page_entry(void* virt_addr);

bool map_table_entry(void* phys_addr, void* virt_addr, u16 hw_flags);

bool map_page_entry(void* phys_addr, void* virt_addr, u16 hw_flags);

u16 pfn_to_hw_flags(u16 flags);

static inline void invlpg(void* va) 
{
    asm volatile("invlpg (%0)" :: "r"(va) : "memory");
}
static inline void flush_tlb()
{
    usize_ptr cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %0, %%cr3" :: "r"(cr3) : "memory");
}
#endif // __RAW_VIRT_ALLOC_H__