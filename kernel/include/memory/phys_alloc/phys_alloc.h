#ifndef __PHYS_ALLOC_H__
#define __PHYS_ALLOC_H__

#include <stdint.h>


enum phys_page_type {
    PAGETYPE_NONE = 0,
    PAGETYPE_1MiB,
    PAGETYPE_VENTRY,
    PAGETYPE_RESERVED,
    PAGETYPE_UNUSED, // not physically allocated
    PAGETYPE_ACPI, 
    PAGETYPE_MMIO,
    PAGETYPE_KERNEL,
    PAGETYPE_HEAP,
    PAGETYPE_PHYS_PAGES,
    PAGETYPE_DISK_CACHE,
};

enum phys_page_flag {
    PAGEFLAG_VFREE    = 1 << 0, // Virtually free (Anything uses this memory, that's not heap's buddy)
    PAGEFLAG_BUDDY    = 1 << 1, // Buddy(1) OR Slab(0)
    PAGEFLAG_HEAD     = 1 << 2, // Is the head in contiguous virtual allocation
    PAGEFLAG_DRIVER   = 1 << 3, // Driver related page
    PAGEFLAG_KERNEL   = 1 << 4, // KERNEL or USER
    PAGEFLAG_READONLY = 1 << 5, // Readonly 
    PAGEFLAG_NOEXEC   = 1 << 6, // No Execute  
    PAGEFLAG_IDEN_MAP = 1 << 7, // Identity Mapped  
};

extern uintptr_t max_memory;
enum phys_page_type;

typedef struct phys_alloc
{
    void*     addr;
    uintptr_t count;
} phys_alloc_t;

extern void* (*alloc_phys_page)(enum phys_page_type type, uint16_t flags);
extern phys_alloc_t (*alloc_phys_pages)(const uintptr_t count, enum phys_page_type type, uint16_t flags);
extern void (*free_phys_page)(void* pa);
extern void (*free_phys_pages)(phys_alloc_t free_params);

#endif // __PHYS_ALLOC_H__