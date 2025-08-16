#ifndef __HEAP_H__
#define __HEAP_H__

#include <core/defs.h>
#include <core/paging.h>

#define SLAB_MIN_SIZE 16

#define SLOTS_PER_PAGE (PAGE_SIZE / SLAB_MIN_SIZE)
#define BITMAP_LEN (SLOTS_PER_PAGE / BIT_TO_BYTE)

#define INVALID_MEMORY ~0

typedef enum page_types
{
    page_type_available = 1,
    
    page_type_heap, /*Undefined heap*/
    page_type_heap_buddy,
    page_type_heap_slab,

    page_type_kernel_static,/*can't be ever freed*/
} page_types_t;

typedef struct phys_page {
    void* struct_addr;    
    uint8_t type;   
    uint8_t flags;  
    uint16_t extra; 
} phys_page_t;
typedef struct phys_page_list {
    phys_page_t* data;
    uint32_t length;
} phys_page_list_t;


void setup_heap(void* heap_addr, uint32_t init_size);

#endif // __HEAP_H__