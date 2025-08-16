#ifndef __PAGE_FRAME_H__
#define __PAGE_FRAME_H__

#include <stdint.h>
#include <memory/phys_alloc.h>

enum phys_page_type {
    PAGE_NONE = 0,
    PAGE_1MiB,
    PAGE_DIRECTORY,
    PAGE_TABLE,
    PAGE_RESERVED,
    PAGE_FREE,
    PAGE_KERNEL,
    PAGE_HEAP,
    PAGE_PHYS_PAGES,
};

typedef struct phys_page_descriptor {
     
    uint32_t ref_count;
    uint32_t flags;

    union {
        // Buddy allocator info
        struct {
            void* prev;
            void* next;
        } buddy;

        // Slab allocator info
        struct {
            void* free_head;
            uint16_t in_use;
            uint16_t objects;
            void* slab_cache;
        } slab;

        // Free page info
        struct {
            // both NULL unless is head
            struct phys_page_descriptor* next_desc;
            struct phys_page_descriptor* prev_desc;
            // real count in head and foot
            uint32_t count;
        } free_page;
    } u;
    
    uint16_t type; 
} phys_page_descriptor_t;

extern phys_page_descriptor_t* phys_page_descs;
void* alloc_phys_page_pfn(enum phys_page_type type);

uint32_t setup_page_descriptors(uint32_t alloc_addr, multiboot_info_t* mbd);
phys_page_descriptor_t* virt_to_pfn(void* addr);
#endif // __PAGE_FRAME_H__