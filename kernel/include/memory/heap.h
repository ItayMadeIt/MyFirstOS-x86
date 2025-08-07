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

typedef struct page {
    void* struct_addr;    
    uint8_t type;   
    uint8_t flags;  
    uint16_t extra; 
} page_t;


typedef struct slab_node
{
    uint32_t addr;
    uint16_t free_bits;    
    uint8_t bitmap[BITMAP_LEN];
    
    struct slab_node* next;

} slab_node_t;

typedef struct page_slab_metadata
{
    slab_node_t* slab_node; // null = isn't allocated
    uint16_t slab_size; 
    uint16_t unused; 

} page_slab_metadata_t;

typedef struct slab_alloc_blocks
{
    uint32_t slab_size; // 2^n

    slab_node_t* head;

} slab_alloc_blocks_t;

typedef struct buddy_node
{
    uint32_t addr; // page aligned so first few bits are used
    struct buddy_node* next;
} buddy_node_t;

typedef struct page_buddy_metadata
{
    uint32_t buddy_size;
    buddy_node_t* buddy_node;
} page_buddy_metadata_t __attribute__((aligned(sizeof(uint32_t))));

typedef struct buddy_alloc_blocks
{
    // buddy attr
    uint32_t buddy_size; // 2^n

    buddy_node_t* head;

} buddy_alloc_blocks_t;

typedef struct heap_metadata
{
    uint32_t size;
} heap_metadata_t;

void setup_heap();

#endif // __HEAP_H__