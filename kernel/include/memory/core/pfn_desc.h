#ifndef __PAGE_FRAME_H__
#define __PAGE_FRAME_H__

#include <stdint.h>
#include <memory/heap/heap_structs.h>
#include <memory/phys_alloc/bitmap_alloc.h>


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

void init_pfn_descriptors(void** alloc_addr, boot_data_t* mbd);

typedef struct pfn_manager_data {
    phys_page_descriptor_t* descs;
    uintptr_t count;
} pfn_manager_data_t;

extern pfn_manager_data_t pfn_data;

phys_page_descriptor_t* pfn_map_page(void* phys_addr, uint16_t page_type, uint16_t page_flags);
phys_page_descriptor_t* phys_to_pfn(void* addr);
phys_page_descriptor_t* virt_to_pfn(void* addr);

#endif // __PAGE_FRAME_H__