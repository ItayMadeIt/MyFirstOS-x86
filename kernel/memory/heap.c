#include <core/defs.h>
#include "core/debug.h"
#include "memory/heap_structs.h"
#include "memory/page_frame.h"
#include <memory/heap.h>
#include <memory/virt_alloc.h>
#include <string.h>

#define BUDDY_BLOCK_SIZE_MIN PAGE_SIZE 
#define BUDDY_EXPON_MAX 31 /*2^31*/ 
#define BUDDY_EXPON_MIN 12 /*2^12*/
#define BUDDY_ORDER_MAX (BUDDY_EXPON_MAX - BUDDY_EXPON_MIN)/*31-12=19*/
#define BUDDY_ORDER_COUNT (BUDDY_EXPON_MAX - BUDDY_EXPON_MIN + 1) 

#define SLAB_ORDER_MAX 11 /*2^11*/ 
#define SLAB_ORDER_MIN 4  /*2^4*/
#define SLAB_INDEX_ORDER_MAX (SLAB_ORDER_MAX - SLAB_ORDER_MIN)/*11-4=7*/
#define SLAB_ORDER_COUNT (SLAB_ORDER_MAX - SLAB_ORDER_MIN + 1) 

typedef struct heap_vars
{
    heap_buddy_order_t* free_buddies[BUDDY_ORDER_COUNT];
    heap_slab_order_t*  free_slabs  [SLAB_ORDER_COUNT];

    void* min_addr;
    void* max_addr;
    uint32_t size;
} heap_vars_t;

heap_vars_t heap;


void* (*kmalloc)(uint32_t size);
void* (*krealloc)(uint32_t new_size);
void  (*kfree)(void* ptr);


static phys_page_descriptor_t* resolve_merge(phys_page_descriptor_t* cur_desc, uint32_t order)
{
    heap_buddy_order_t* cur_desc_order = &cur_desc->u.buddy.order;

    uint32_t expon = order + BUDDY_EXPON_MIN;

    // Check if neighbour exists
    uint32_t neighbour_va = cur_desc_order->virt_addr ^ (1<<expon);
    phys_page_descriptor_t* neighbour_desc = virt_to_pfn((void*)neighbour_va);

    uint32_t buddy_flags = (PAGEFLAG_BUDDY | PAGEFLAG_VFREE | PAGEFLAG_HEAD);
    if (neighbour_desc->type != PAGETYPE_HEAP ||
        (neighbour_desc->flags & buddy_flags) != buddy_flags ||
        neighbour_desc->num_pages != cur_desc->num_pages)
    {
        // No neighbour
        return NULL;
    }

    // addr for the result desc is the lower one
    void* addr = (void*)min(neighbour_va, cur_desc_order->virt_addr);
    phys_page_descriptor_t* new_desc = (uint32_t)addr == neighbour_va ? neighbour_desc : cur_desc;
    phys_page_descriptor_t* old_desc = (uint32_t)addr == neighbour_va ? cur_desc : neighbour_desc;
    heap_buddy_order_t* new_desc_order = &new_desc->u.buddy.order;
    heap_buddy_order_t* old_desc_order = &old_desc->u.buddy.order;

    old_desc->flags &= ~(PAGEFLAG_HEAD);

    // Remove new_desc from linked list
    if (new_desc_order->next_free)
        new_desc_order->next_free->prev_free = new_desc_order->prev_free; 
    if (new_desc_order->prev_free)
        new_desc_order->prev_free->next_free = new_desc_order->next_free; 
    if (heap.free_buddies[order] == new_desc_order)
        heap.free_buddies[order] = new_desc_order->next_free;

    // Remove old_desc from linked list
    if (old_desc_order->next_free)
        old_desc_order->next_free->prev_free = old_desc_order->prev_free; 
    if (old_desc_order->prev_free)
        old_desc_order->prev_free->next_free = old_desc_order->next_free; 
    if (heap.free_buddies[order] == old_desc_order)
        heap.free_buddies[order] = old_desc_order->next_free;

    uint32_t new_order = order + 1;
    assert(new_order < BUDDY_ORDER_COUNT);

    // Insert at the new order
    new_desc_order->next_free = heap.free_buddies[new_order];
    new_desc_order->prev_free = NULL;
    if (heap.free_buddies[new_order])
        heap.free_buddies[new_order]->prev_free = new_desc_order;
    heap.free_buddies[new_order] = new_desc_order;

    // Update new_desc metadata
    new_desc->u.buddy.buddy_cache = &heap.free_buddies[new_order];
    new_desc->u.buddy.order.buddy_order = new_order;
    new_desc->num_pages *= 2;

    return new_desc;
}

static void add_buddies(uint32_t heap_addr, uint32_t init_size, bool is_init)
{
    uint32_t remaining_size = init_size;
    
    while (remaining_size)
    {
        uint32_t order = min(log2_u32(remaining_size)-BUDDY_EXPON_MIN, BUDDY_ORDER_MAX);
        uint32_t expon = order + BUDDY_EXPON_MIN; 

        uint32_t cur_size = 1 << expon;

        while ((heap_addr & (cur_size - 1)) != 0 && order)
        {
            order--;
            expon--;

            cur_size = 1 << expon;
        } 

        if (order == 0)
        {
            assert(heap_addr % PAGE_SIZE == 0);
        }

        phys_page_descriptor_t* page_desc = virt_to_pfn((void*)heap_addr);
        
        assert(page_desc->type == PAGETYPE_HEAP);

        page_desc->flags |= PAGEFLAG_HEAD; 
        page_desc->u.buddy.buddy_cache = &heap.free_buddies[order];
        page_desc->u.buddy.order.buddy_order = order;
        page_desc->u.buddy.order.virt_addr = heap_addr;

        // insert at beginning        
        page_desc->u.buddy.order.prev_free = NULL;
        if (heap.free_buddies[order])
            heap.free_buddies[order]->prev_free = &page_desc->u.buddy.order;

        page_desc->u.buddy.order.next_free = heap.free_buddies[order];
        
        heap.free_buddies[order] = &page_desc->u.buddy.order;

        // Set num pages
        page_desc->num_pages = cur_size / PAGE_SIZE;

        remaining_size -= cur_size;
        heap_addr += cur_size;

        if (!is_init)
        {
            resolve_merge(page_desc, order);
        }
    }
}


static void add_heap_region(uint32_t new_size)
{
    uint32_t add_size = new_size - heap.size;
    assert(add_size && add_size % PAGE_SIZE == 0);
    
    assert(map_pages(
        heap.max_addr, add_size/PAGE_SIZE, 
        PAGETYPE_HEAP, 
        PAGEFLAG_VFREE | PAGEFLAG_BUDDY | PAGEFLAG_KERNEL
    ));
    
    add_buddies((uint32_t)heap.max_addr, add_size, false);

    heap.max_addr += add_size;
}

void init_heap(uint32_t size, uint32_t start_va)
{
    heap.size = size;
    
    for (uint32_t i = 0; i < BUDDY_ORDER_COUNT; i++)
    {
        heap.free_buddies[i] = NULL;
    }

    for (uint32_t i = 0; i < SLAB_ORDER_COUNT; i++)
    {
        heap.free_slabs[i] = NULL;
    }

    heap.min_addr = (void*) start_va;
    heap.max_addr = (void*)(start_va + size);
}


void* alloc_buddy(uint32_t size)
{
    assert(size && size % PAGE_SIZE == 0);
    assert((size & (size-1)) == 0);

    uint32_t expon = log2_u32(size);
    uint32_t order = expon - BUDDY_EXPON_MIN;

    uint32_t og_order = order;

    while (order <= BUDDY_ORDER_MAX && heap.free_buddies[order] == NULL)
    {
        order++;
        expon++;
    }

    if (order > BUDDY_ORDER_MAX || heap.free_buddies[order] == NULL)
    {
        if (order > BUDDY_ORDER_MAX)
        {
            // Currently I want to make sure no more allocations than the init size are used 
            // (if so I just want to see and fix overusaged)
            debug_print_str("Heap allocated more (INIT_SIZE wasn't sufficient) [buddy allocator]\n");
            abort();

            add_heap_region(size * 2);
            return alloc_buddy(size);
        } 

        return NULL;
    }

    while (order != og_order)
    {
        heap_buddy_order_t* cur_order_obj = heap.free_buddies[order]; 

        // Remove the first buddy in free_buddies[order]
        if (cur_order_obj->next_free)
            cur_order_obj->next_free->prev_free = NULL;

        heap.free_buddies[order] = cur_order_obj->next_free;

        order--;
        expon--;

        phys_page_descriptor_t* cur_page_desc = virt_to_pfn((void*)cur_order_obj->virt_addr);

        uint32_t neighbor_va = cur_order_obj->virt_addr + (1 << expon);
        phys_page_descriptor_t* neighbor_page_desc = virt_to_pfn((void*)neighbor_va);

        cur_page_desc->num_pages /= 2;
        cur_page_desc->u.buddy.order.buddy_order = order;
        cur_page_desc->u.buddy.buddy_cache = &heap.free_buddies[order];

        // Make the neighbor from nothing
        neighbor_page_desc->flags &= ~PAGEFLAG_HEAP_MASK;
        neighbor_page_desc->flags |= PAGEFLAG_VFREE | PAGEFLAG_BUDDY | PAGEFLAG_HEAD;
        
        neighbor_page_desc->type  = PAGETYPE_HEAP;
        neighbor_page_desc->num_pages =  cur_page_desc->num_pages;
        neighbor_page_desc->u.buddy.buddy_cache = &heap.free_buddies[order];

        // Set the neighbor order
        heap_buddy_order_t* neighbor_order = &neighbor_page_desc->u.buddy.order;
        neighbor_order->buddy_order = order;
        neighbor_order->virt_addr = neighbor_va;
        neighbor_order->next_free = NULL;
        neighbor_order->prev_free = NULL;
        
        // push current
        cur_order_obj->next_free = heap.free_buddies[order];
        cur_order_obj->prev_free = NULL;
        if (heap.free_buddies[order])
            heap.free_buddies[order]->prev_free = cur_order_obj;
        heap.free_buddies[order] = cur_order_obj;

        // push neighbor
        neighbor_order->next_free = heap.free_buddies[order];
        neighbor_order->prev_free = NULL;
        if (heap.free_buddies[order])
            heap.free_buddies[order]->prev_free = neighbor_order;
        heap.free_buddies[order] = neighbor_order;
    }

    heap_buddy_order_t* cur_order_obj = heap.free_buddies[order]; 
    heap.free_buddies[order] = heap.free_buddies[order]->next_free;

    phys_page_descriptor_t* cur_page_desc = virt_to_pfn((void*)cur_order_obj->virt_addr);
    cur_page_desc->flags &= ~PAGEFLAG_VFREE; // it's not free anymore

    if (heap.free_buddies[order])
        heap.free_buddies[order]->prev_free = NULL;

    return (void*)cur_order_obj->virt_addr;
}

void free_buddy(void* addr)
{
    assert((uint32_t)addr % PAGE_SIZE == 0);
    
    phys_page_descriptor_t* cur_desc = virt_to_pfn(addr);
    
    assert((cur_desc->num_pages & (cur_desc->num_pages-1)) == 0 && cur_desc->num_pages > 0);
    assert(cur_desc->flags & PAGEFLAG_BUDDY);
    assert(cur_desc->flags & PAGEFLAG_HEAD);
    assert((cur_desc->flags & PAGEFLAG_VFREE) == 0);

    uint32_t order = log2_u32(cur_desc->num_pages);

    assert(cur_desc->u.buddy.buddy_cache == &heap.free_buddies[order]);

    cur_desc->flags |= PAGEFLAG_VFREE;

    // insert at the beginning
    heap_buddy_order_t* cur_desc_order = &cur_desc->u.buddy.order;
    if (heap.free_buddies[order])
    {
        heap.free_buddies[order]->prev_free = cur_desc_order;
    }
    cur_desc_order->next_free = heap.free_buddies[order];
    cur_desc_order->prev_free = NULL;
    heap.free_buddies[order] = cur_desc_order;

    // Merge upwards
    while (cur_desc)
    {
        cur_desc = resolve_merge(cur_desc, order);
        order++;
    }
}

void setup_heap(void* heap_addr, uint32_t init_size)
{
    assert((uint32_t)heap_addr % PAGE_SIZE == 0);
    assert(init_size != 0);
    assert(init_size % PAGE_SIZE == 0);
    assert(init_size < (1u << BUDDY_EXPON_MAX));

    kmalloc  = (void* (*)(uint32_t))abort;
    krealloc = (void* (*)(uint32_t))abort;
    kfree    = (void  (*)(void*   ))abort;

    init_heap(init_size, (uint32_t)heap_addr);

    assert(map_pages(
        heap_addr, 
        init_size/PAGE_SIZE,
        PAGETYPE_HEAP, 
        PAGEFLAG_VFREE|PAGEFLAG_BUDDY|PAGEFLAG_KERNEL
    ));
    heap.max_addr = heap_addr + init_size;

    add_buddies((uint32_t)heap_addr, init_size, true);

    void* p1 = alloc_buddy(PAGE_SIZE * 2);
    void* p4 = alloc_buddy(PAGE_SIZE * 2);
    free_buddy(p4); 
    void* p3 = alloc_buddy(PAGE_SIZE * 4);
    void* p2 = alloc_buddy(PAGE_SIZE);
    free_buddy(p1); 
    free_buddy(p2); 
    free_buddy(p3); 

    /*
    Still does nothing.
    kmalloc ;
    krealloc ;
    kfree ;
    */
}