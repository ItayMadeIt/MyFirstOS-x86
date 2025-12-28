#ifndef __PAGE_FRAME_H__
#define __PAGE_FRAME_H__

#include "core/num_defs.h"
#include "kernel/boot/boot_data.h"
#include <memory/heap/heap_structs.h>

enum phys_page_flag {   
    PAGEFLAG_LOCKED    = 1 << 0,
    PAGEFLAG_DIRTY     = 1 << 1,
    PAGEFLAG_RESERVED  = 1 << 2
};

enum phys_page_type {
    PAGETYPE_NONE = 0,
    PAGETYPE_1MiB,
    PAGETYPE_RESERVED,
    PAGETYPE_UNUSED, // not physically allocated
    PAGETYPE_ACPI, 
    PAGETYPE_MMIO,
    PAGETYPE_KERNELIMG,
    PAGETYPE_PFN,
    PAGETYPE_DRIVER,
    PAGETYPE_PHYS_PAGES, // pte/pde pages (or the arch's equivalent)
    PAGETYPE_USED,
};


//#define PAGEFLAG_HEAP_MASK (PAGEFLAG_VFREE | PAGEFLAG_BUDDY | PAGEFLAG_HEAD)

typedef struct page {
     
    u32 ref_count;
    u16 flags;
    u16 type; 

    union {
        // Heap objects
        heap_page_metadata_t heap;

        // Free page info
        struct {
            // both NULL unless is head
            struct page* next_desc;
            struct page* prev_desc;
            // real count in head and foot
            u32 count;
        } free_page;

    } u;
    
} page_t;

usize_ptr pfn_page_count();
usize_ptr pfn_page_free_count();

void pfn_free_amount (usize_ptr amount);
void pfn_alloc_amount(usize_ptr amount);
 
void pfn_get_page(page_t* desc);
void pfn_put_page(page_t* desc);

void pfn_get_range(page_t* begin, usize_ptr count);
void pfn_put_range(page_t* begin, usize_ptr count);

void init_pfn_descriptors(void** alloc_addr, boot_data_t* mbd);

typedef struct pfn_manager_data {
    page_t* descs;
    usize_ptr count;
    usize_ptr free_count;
} pfn_manager_data_t;

void pfn_mark_range(
    void* start_pa, 
    usize_ptr count, 
    enum phys_page_type pfn_type, 
    u16 pfn_flags
);

void pfn_identity_range(
    void* pa,
    usize_ptr count,
    enum phys_page_type pfn_type,
    u16 pfn_flags
);

void pfn_set_type(page_t* desc, enum phys_page_type type);
void pfn_range_set_type    (page_t* desc, usize_ptr count, enum phys_page_type type);

page_t* page_index_to_pfn(usize_ptr pfn_index);
page_t* pa_to_pfn(void* pa);

void*       pfn_to_pa(page_t* desc);
usize_ptr   get_pfn_index(page_t* desc);

#endif // __PAGE_FRAME_H__