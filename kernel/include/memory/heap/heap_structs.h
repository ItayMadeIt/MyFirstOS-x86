#ifndef __HEAP_STRUCTS_H__
#define __HEAP_STRUCTS_H__

#include "core/num_defs.h"

struct phys_page_descriptor;

typedef struct heap_buddy_order
{
    usize_ptr virt_addr;
    usize_ptr buddy_order;
    struct heap_buddy_order* prev_free;
    struct heap_buddy_order* next_free;
} heap_buddy_order_t;

typedef struct heap_buddy_page_metadata
{
    usize_ptr num_pages; 
    heap_buddy_order_t order;
    struct heap_buddy_order** buddy_cache;
    
} heap_buddy_page_metadata_t;

typedef struct __attribute__((packed)) heap_slab_node
{
    struct heap_slab_node* prev_free;
    struct heap_slab_node* next_free;    
} heap_slab_node_t;

struct heap_slab_page_metadata;

typedef struct heap_slab_order
{
    usize_ptr slab_order;
    usize_ptr obj_size;
    usize_ptr slab_size;
    struct heap_slab_page_metadata* free_slab;
} heap_slab_order_t;
typedef struct heap_slab_cache 
{
    heap_slab_order_t order;
    const char* name;
} heap_slab_cache_t;

typedef struct heap_slab_page_metadata
{
    u32 num_pages; // never more than 2^32 pages in order
    u16 obj_count; 
    u16 used_count;

    struct heap_slab_page_metadata* prev_slab;
    struct heap_slab_page_metadata* next_slab;

    struct heap_slab_node* free_node;
    
    struct heap_slab_order* order_cache;
} heap_slab_page_metadata_t;


#endif // __HEAP_STRUCTS_H__