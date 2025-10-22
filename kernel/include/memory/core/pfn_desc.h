#ifndef __PAGE_FRAME_H__
#define __PAGE_FRAME_H__

#include "memory/phys_alloc/phys_alloc.h"
#include <stdint.h>
#include <memory/heap/heap_structs.h>
#include <memory/phys_alloc/bitmap_alloc.h>

enum phys_page_flag {
    PAGEFLAG_NONE     = 0,
    PAGEFLAG_VFREE    = 1 << 0, // Virtually free (Anything uses this memory, that's not heap's buddy)
    PAGEFLAG_BUDDY    = 1 << 1, // Buddy(1) OR Slab(0)
    PAGEFLAG_HEAD     = 1 << 2, // Is the head in contiguous virtual allocation
    PAGEFLAG_DRIVER   = 1 << 3, // Driver related page
    PAGEFLAG_KERNEL   = 1 << 4, // KERNEL or USER
    PAGEFLAG_READONLY = 1 << 5, // Readonly 
    PAGEFLAG_NOEXEC   = 1 << 6, // No Execute  
    PAGEFLAG_IDEN_MAP = 1 << 7, // Identity Mapped  
};

enum phys_page_type {
    PAGETYPE_NONE = 0,
    PAGETYPE_1MiB,
    PAGETYPE_VENTRY,
    PAGETYPE_RESERVED,
    PAGETYPE_UNUSED, // not physically allocated
    PAGETYPE_ACPI, 
    PAGETYPE_MMIO,
    PAGETYPE_KERNEL,
    PAGETYPE_PFN,
    PAGETYPE_USER,
    PAGETYPE_HEAP,
    PAGETYPE_DRIVER,
    PAGETYPE_PHYS_PAGES,
    PAGETYPE_DISK_CACHE,
};


#define PAGEFLAG_HEAP_MASK (PAGEFLAG_VFREE | PAGEFLAG_BUDDY | PAGEFLAG_HEAD)

typedef struct phys_page_descriptor {
     
    uint32_t ref_count;
    uint16_t flags;
    uint16_t type; 

    union {
        // Heap objects
        heap_slab_page_metadata_t  slab;
        struct phys_page_descriptor* slab_head;
        heap_buddy_page_metadata_t buddy;

        // Free page info
        struct {
            // both NULL unless is head
            struct phys_page_descriptor* next_desc;
            struct phys_page_descriptor* prev_desc;
            // real count in head and foot
            uint32_t count;
        } free_page;
    } u;
    
} phys_page_descriptor_t;

void pfn_ref_page(void* pa);
void pfn_unref_page(void* pa);
void pfn_ref_range(void* pa, uintptr_t count);
void pfn_unref_range(void* pa, uintptr_t count);

void pfn_ref_vpage(void* va);
void pfn_unref_vpage(void* va);
void pfn_ref_vrange(void* va, uintptr_t count);
void pfn_unref_vrange(void* va, uintptr_t count);

void init_pfn_descriptors(void** alloc_addr, boot_data_t* mbd);

typedef struct pfn_manager_data {
    phys_page_descriptor_t* descs;
    uintptr_t count;
} pfn_manager_data_t;

extern pfn_manager_data_t pfn_data;

void pfn_mark_range(
    void* start_pa, 
    uintptr_t count, 
    enum phys_page_type pfn_type, 
    uint16_t pfn_flags
);

void pfn_mark_vrange(
    void* start_va, 
    uintptr_t count, 
    enum phys_page_type pfn_type, 
    uint16_t pfn_flags
);

void pfn_alloc_map_pages(
    void* va,
    uintptr_t count, 
    enum phys_page_type pfn_type,
    uint16_t pfn_flags
);

void pfn_alloc_map_page (
    void* va,
    enum phys_page_type pfn_type,
    uint16_t pfn_flags
);

void pfn_clone_map(
    void* dst_va,
    void* src_va,
    uintptr_t count
);

void pfn_share_map(
    void* dst_va,
    void* src_va,
    uintptr_t count,
    uint16_t dst_pfn_flags // doesn't modify the PFN descriptor flags
);

void pfn_identity_map_pages(
    void* pa, 
    uintptr_t count,
    enum phys_page_type pfn_type,
    uint16_t pfn_flags
);

void pfn_map_pages(
    void* pa,
    void* va,
    uintptr_t count,
    enum phys_page_type type,
    uint16_t flags
);


void pfn_unmap_pages(
    void* va, 
    uintptr_t count
);
void pfn_unmap_page (
    void* va
);

phys_page_descriptor_t* phys_to_pfn(void* pa);
phys_page_descriptor_t* virt_to_pfn(void* va);

#endif // __PAGE_FRAME_H__