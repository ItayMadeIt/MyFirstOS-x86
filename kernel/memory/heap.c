#include "memory/page_frame.h"
#include <memory/heap.h>
#include <memory/virt_alloc.h>

#define BUDDY_BLOCK_SIZE_MIN PAGE_SIZE 
#define BUDDY_ORDER_MAX 25 /*2^25*/ 
#define BUDDY_ORDER_MIN 12 /*2^12*/
#define BUDDY_ORDER_COUNT (BUDDY_ORDER_MAX - BUDDY_ORDER_MIN + 1) 

#define SLAB_ORDER_MAX 11 /*2^11*/ 
#define SLAB_ORDER_MIN 5  /*2^5*/
#define SLAB_ORDER_COUNT (SLAB_ORDER_MAX - SLAB_ORDER_MIN + 1) 

typedef struct buddy_node
{
    uint32_t addr;
    struct buddy_node* next;  
} buddy_node_t;

typedef struct buddy_order 
{
    uint32_t size;
    buddy_node_t* free_node;
    
} buddy_order_t;

typedef struct heap_vars
{
   buddy_order_t free_buddies[BUDDY_ORDER_COUNT];
   //slab_order_t free_slabs  [SLAB_ORDER_COUNT];

} heap_vars_t;

heap_vars_t heap;


void* (*malloc)(uint32_t size);
void* (*realloc)(uint32_t size);
void (*free)(void* ptr);

void init_heap_vars()
{
    for (uint32_t i = 0; i < BUDDY_ORDER_COUNT; i++)
    {
        heap.free_buddies[i].free_node = NULL;
        heap.free_buddies[i].size = 0;
    }
}

void setup_heap(void* heap_addr, uint32_t init_size)
{
    assert(init_size % PAGE_SIZE == 0);

    init_heap_vars();

    assert(map_pages(
        heap_addr, 
        init_size/PAGE_SIZE, 
        PAGE_ENTRY_WRITE_KERNEL_FLAGS,
         PAGE_HEAP)
    );

    /*
    Still does nothing.
    malloc = early_alloc;
    free = early_free;
    */
}