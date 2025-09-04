#ifndef __HEAP_STRUCTS_H__
#define __HEAP_STRUCTS_H__

#include <stdint.h>

struct phys_page_descriptor;

typedef struct heap_buddy_order
{
    uint32_t virt_addr;
    uint32_t buddy_order;
    struct heap_buddy_order* prev_free;
    struct heap_buddy_order* next_free;
} heap_buddy_order_t;

typedef struct heap_buddy_page_metadata
{
    uint32_t num_pages; 
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
    uint32_t slab_order;
    uint32_t obj_size;
    uint32_t slab_size;
    struct heap_slab_page_metadata* free_slab;
} heap_slab_order_t;
typedef struct heap_slab_cache 
{
    heap_slab_order_t order;
    const char* name;
} heap_slab_cache_t;

typedef struct heap_slab_page_metadata
{
    uint32_t num_pages; 
    uint32_t obj_count; 
    uint32_t used_count;

    struct heap_slab_page_metadata* prev_slab;
    struct heap_slab_page_metadata* next_slab;

    struct heap_slab_node* free_node;
    
    struct heap_slab_order* order_cache;
} heap_slab_page_metadata_t;


#endif // __HEAP_STRUCTS_H__