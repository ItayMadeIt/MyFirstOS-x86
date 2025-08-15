#include "memory/virt_alloc.h"
#include <memory/heap.h>

#define BUDDY_BLOCK_SIZE_MIN PAGE_SIZE 
#define BUDDY_ORDER_MAX 25 /*2^25*/ 
#define BUDDY_ORDER_MIN 12 /*2^12*/
#define BUDDY_ORDER_COUNT (BUDDY_ORDER_MAX - BUDDY_ORDER_MIN+1) 

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
   buddy_order_t buddies[BUDDY_ORDER_COUNT];

} heap_vars_t;

heap_vars_t heap;


void* (*malloc)(uint32_t size);
void* (*realloc)(uint32_t size);
void (*free)(void* ptr);

void setup_heap(uint32_t heap_addr)
{
    /*
    Still does nothing.

    init_early_heap();
    malloc = early_alloc;
    free = early_free;
    */
}