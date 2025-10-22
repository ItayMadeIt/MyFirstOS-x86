#include <core/defs.h>
#include <kernel/memory/paging.h>
#include <memory/heap/heap_structs.h>
#include <memory/heap/heap.h>
#include <memory/core/pfn_desc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BUDDY_BLOCK_SIZE_MIN PAGE_SIZE 
#define BUDDY_EXPON_MAX 31 /*2^31*/ 
#define BUDDY_EXPON_MIN 12 /*2^12*/
#define BUDDY_ORDER_MAX (BUDDY_EXPON_MAX - BUDDY_EXPON_MIN)/*31-12=19*/
#define BUDDY_ORDER_COUNT (BUDDY_EXPON_MAX - BUDDY_EXPON_MIN + 1) 

#define SLAB_EXPON_MAX 16 // 2^16
#define SLAB_EXPON_MIN 4  // 2^4
#define SLAB_INDEX_ORDER_MAX (SLAB_EXPON_MAX - SLAB_EXPON_MIN) // 16-4=12
#define SLAB_ORDER_COUNT (SLAB_EXPON_MAX - SLAB_EXPON_MIN + 1) // 12+1=13
#define SLAB_CUSTOM_ORDER (SLAB_ORDER_COUNT+1)

#define MIN_SLAB_OBJ_AMOUNT 32 // At least 32 objects per slab 
#define MAX_SLAB_PAGE_AMOUNT 64 // Max of 64 pages (64 * 4KiB = 256KiB)

typedef struct heap_vars
{
    heap_buddy_order_t* free_buddies[BUDDY_ORDER_COUNT];
    heap_slab_order_t  free_slabs  [SLAB_ORDER_COUNT];

    void* min_addr;
    void* cur_max_addr;
    void* reserved_max_addr;
    uintptr_t cur_size;
    uintptr_t reserved_max_size;
} heap_vars_t;

heap_vars_t heap;

static phys_page_descriptor_t* resolve_merge(phys_page_descriptor_t* cur_desc, uint8_t order)
{
    heap_buddy_order_t* cur_desc_order = &cur_desc->u.buddy.order;

    uint8_t expon = order + BUDDY_EXPON_MIN;

    // Check if neighbour exists
    uintptr_t neighbour_va = cur_desc_order->virt_addr ^ (1<<expon);
    phys_page_descriptor_t* neighbour_desc = virt_to_pfn((void*)neighbour_va);

    uint16_t buddy_flags = (PAGEFLAG_BUDDY | PAGEFLAG_VFREE | PAGEFLAG_HEAD);

    if (neighbour_desc->type != PAGETYPE_HEAP ||
        (neighbour_desc->flags & buddy_flags) != buddy_flags ||
        neighbour_desc->u.buddy.num_pages != cur_desc->u.buddy.num_pages)
    {
        // No neighbour
        return NULL;
    }

    // addr for the result desc is the lower one
    void* addr = (void*)min(neighbour_va, cur_desc_order->virt_addr);
    phys_page_descriptor_t* new_desc = (uintptr_t)addr == neighbour_va ? neighbour_desc : cur_desc;
    phys_page_descriptor_t* old_desc = (uintptr_t)addr == neighbour_va ? cur_desc : neighbour_desc;
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

    old_desc_order->next_free = NULL;
    old_desc_order->prev_free = NULL;

    uint8_t new_order = order + 1;
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
    new_desc->u.buddy.num_pages *= 2;

    return new_desc;
}

static void merge_upwards(phys_page_descriptor_t* cur_desc, uint8_t order)
{
    while (cur_desc)
    {
        cur_desc = resolve_merge(cur_desc, order); 
        order++;
    }
}

static void add_buddies(uintptr_t heap_addr, uintptr_t init_size, bool is_init)
{
    uintptr_t remaining_size = init_size;
    
    while (remaining_size)
    {
        uint8_t order = min(log2_u32(remaining_size)-BUDDY_EXPON_MIN, BUDDY_ORDER_MAX);
        uint8_t expon = order + BUDDY_EXPON_MIN; 

        uintptr_t cur_size = 1 << expon;

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
        page_desc->u.buddy.num_pages = cur_size / PAGE_SIZE;

        remaining_size -= cur_size;
        heap_addr += cur_size;

        if (!is_init)
        {
            merge_upwards(page_desc, order);
        }
    }
}


static void add_heap_region(uintptr_t add_size)
{
    assert(add_size && add_size % PAGE_SIZE == 0);
    
    if (heap.cur_max_addr + add_size > heap.reserved_max_addr)
    {
        printf("Can't grow heap more");
        abort();
        return;
    }

    pfn_alloc_map_pages(
        heap.cur_max_addr, add_size/PAGE_SIZE, 
        PAGETYPE_HEAP, 
        PAGEFLAG_VFREE | PAGEFLAG_BUDDY | PAGEFLAG_KERNEL
    );
    
    add_buddies((uintptr_t)heap.cur_max_addr, add_size, false);

    heap.cur_max_addr += add_size;
    heap.cur_size += add_size;
}

static void grow_heap(uintptr_t failed_size)
{
    uintptr_t grow_size = min(
        max(failed_size, heap.cur_size),
        heap.reserved_max_size - heap.cur_size  // max possible grow size
    );

    if (heap.cur_size + grow_size > heap.reserved_max_size) 
    {
        printf("Heap wasn't sufficient\n");
        abort();
    }

    add_heap_region(grow_size);
}

static uintptr_t pages_for_obj_size(uintptr_t obj_size)
{
    uintptr_t min_total_bytes = obj_size * MIN_SLAB_OBJ_AMOUNT;

    uintptr_t pages_size = 1u << log2_u32(
        max(align_up_pow2(min_total_bytes), PAGE_SIZE)
    );

    uintptr_t pages = pages_size / PAGE_SIZE;

    return min(pages, MAX_SLAB_PAGE_AMOUNT);
}

static void init_heap_vars(uintptr_t size, uintptr_t max_size, uintptr_t start_va)
{
    heap.cur_size = size;
    
    for (uintptr_t i = 0; i < BUDDY_ORDER_COUNT; i++)
    {
        heap.free_buddies[i] = NULL;
    }

    for (uintptr_t i = 0; i < SLAB_ORDER_COUNT; i++)
    {
        heap.free_slabs[i].free_slab = NULL;
        heap.free_slabs[i].obj_size = 1 << (SLAB_EXPON_MIN + i);
        heap.free_slabs[i].slab_order = i;
        heap.free_slabs[i].slab_size = 
            pages_for_obj_size(heap.free_slabs[i].obj_size) * PAGE_SIZE;
    }

    heap.min_addr = (void*) start_va;
    heap.cur_max_addr = (void*)(start_va + size);
    
    heap.reserved_max_addr = (void*)(start_va + max_size);
    heap.reserved_max_size = max_size;
    
}


void* alloc_buddy(uintptr_t size)
{
    assert(size && size % PAGE_SIZE == 0);
    assert((size & (size-1)) == 0);

    uint8_t expon = log2_u32(size);
    uint8_t order = expon - BUDDY_EXPON_MIN;

    uint8_t og_order = order;

    while (order <= BUDDY_ORDER_MAX && heap.free_buddies[order] == NULL)
    {
        order++;
        expon++;
    }

    if (order > BUDDY_ORDER_MAX || heap.free_buddies[order] == NULL)
    {
        if (order > BUDDY_ORDER_MAX)
        {
            printf("Heap allocated more (INIT_SIZE wasn't sufficient) [buddy allocator]\n");
            grow_heap(size);
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

        uintptr_t neighbor_va = cur_order_obj->virt_addr + (1 << expon);
        phys_page_descriptor_t* neighbor_page_desc = virt_to_pfn((void*)neighbor_va);

        cur_page_desc->u.buddy.num_pages /= 2;
        cur_page_desc->u.buddy.order.buddy_order = order;
        cur_page_desc->u.buddy.buddy_cache = &heap.free_buddies[order];

        // Make the neighbor from nothing
        neighbor_page_desc->flags &= ~PAGEFLAG_HEAP_MASK;
        neighbor_page_desc->flags |= PAGEFLAG_VFREE | PAGEFLAG_BUDDY | PAGEFLAG_HEAD;
        
        neighbor_page_desc->type  = PAGETYPE_HEAP;
        neighbor_page_desc->u.buddy.num_pages =  cur_page_desc->u.buddy.num_pages;
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
    assert((uintptr_t)addr % PAGE_SIZE == 0);
    
    phys_page_descriptor_t* cur_desc = virt_to_pfn(addr);
    
    assert((cur_desc->u.buddy.num_pages & (cur_desc->u.buddy.num_pages-1)) == 0 && cur_desc->u.buddy.num_pages > 0);
    assert(cur_desc->flags & PAGEFLAG_BUDDY);
    assert(cur_desc->flags & PAGEFLAG_HEAD);
    assert((cur_desc->flags & PAGEFLAG_VFREE) == 0);

    uint8_t order = log2_u32(cur_desc->u.buddy.num_pages);

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

    cur_desc_order->virt_addr = (uintptr_t)addr;
    cur_desc_order->buddy_order = order;

    merge_upwards(cur_desc, order);
}

static heap_slab_node_t* init_slab_free_list(void* base_addr, uintptr_t obj_size, uint16_t obj_count) 
{
    uintptr_t current_ptr = (uintptr_t) base_addr;
    ((heap_slab_node_t*)current_ptr)->prev_free = NULL;

    uintptr_t last_ptr = current_ptr;
    current_ptr += obj_size;

    for (uint16_t i = 1; i < obj_count; i++) 
    {
        ((heap_slab_node_t*)last_ptr)->next_free = (heap_slab_node_t*)current_ptr;
        ((heap_slab_node_t*)current_ptr)->prev_free = (heap_slab_node_t*)last_ptr;

        last_ptr = current_ptr;
        current_ptr += obj_size;
    }

    ((heap_slab_node_t*)last_ptr)->next_free = NULL;

    return base_addr;
}

void* alloc_slab(heap_slab_order_t* slab_order)
{
    if (slab_order->free_slab)
    {
        heap_slab_page_metadata_t* free_slab = slab_order->free_slab;

        // Remove head
        heap_slab_node_t* used_node = free_slab->free_node;
        free_slab->free_node = used_node ? used_node->next_free : NULL;
        if (free_slab->free_node)
            free_slab->free_node->prev_free = NULL;
        free_slab->used_count++;

        // Remove slab order if full
        if (free_slab->used_count == free_slab->obj_count)
        {
            if (free_slab->next_slab)
                free_slab->next_slab->prev_slab = NULL;

            slab_order->free_slab = free_slab->next_slab;
        }

        return (void*)used_node;
    }

    // Get phys descriptor
    void* buddy_addr = alloc_buddy(slab_order->slab_size);
    phys_page_descriptor_t* cur_desc = virt_to_pfn(buddy_addr);

    cur_desc->flags &= ~PAGEFLAG_BUDDY;
    
    uintptr_t obj_size  = slab_order->obj_size;
    uintptr_t obj_count = slab_order->slab_size / obj_size;
    assert(obj_count);

    // init phys descriptor as slab
    heap_slab_page_metadata_t* slab_metadata = &cur_desc->u.slab;
    slab_metadata->used_count = 0;
    slab_metadata->order_cache = slab_order;
    slab_metadata->obj_count = obj_count;
    slab_metadata->num_pages = cur_desc->u.buddy.num_pages;

    uintptr_t base = (uintptr_t)buddy_addr;
    uintptr_t bytes = slab_metadata->num_pages * PAGE_SIZE;
    for (uintptr_t off = PAGE_SIZE; off < bytes; off += PAGE_SIZE) 
    {
        phys_page_descriptor_t* tail = virt_to_pfn((void*)(base + off));
        tail->flags &= ~(PAGEFLAG_BUDDY | PAGEFLAG_HEAD);
        tail->u.slab_head = cur_desc;
    }

    // Set up memory linked list free 
    init_slab_free_list(buddy_addr, obj_size, obj_count);
    slab_metadata->free_node = buddy_addr;

    // There wasn't a slab so this was chosen, meaning there will never be a next slab
    slab_metadata->next_slab = NULL;    

    slab_order->free_slab = slab_metadata;

    // Remove head
    heap_slab_node_t* used_node = slab_metadata->free_node;
    slab_metadata->free_node = used_node->next_free;
    if (slab_metadata->free_node)
    {
        slab_metadata->free_node->prev_free = NULL;
    }

    slab_metadata->used_count = 1;

    return (void*)used_node;
}

void free_slab(void* addr)
{
    phys_page_descriptor_t* desc = virt_to_pfn(addr);
    heap_slab_page_metadata_t* slab_metadata = &desc->u.slab;
    assert((desc->flags & PAGEFLAG_BUDDY) == 0);
    
    if ((desc->flags & PAGEFLAG_HEAD) == 0)
    {
        desc = desc->u.slab_head;
        slab_metadata = &desc->u.slab;
    }

    heap_slab_order_t* slab_cache = slab_metadata->order_cache;

    assert(
        (slab_cache->slab_order < SLAB_ORDER_COUNT) || 
        slab_cache->slab_order == SLAB_CUSTOM_ORDER
    ); 

    // insert head
    heap_slab_node_t* new_node = addr;
    new_node->next_free = slab_metadata->free_node;
    if (slab_metadata->free_node)
    {
        slab_metadata->free_node->prev_free = new_node;
    }
    slab_metadata->free_node = new_node;

    // Not considered as a page
    if (slab_metadata->used_count == slab_metadata->obj_count)
    {
        // Insert as head
        slab_metadata->prev_slab = NULL;
        slab_metadata->next_slab = slab_cache->free_slab;

        if (slab_metadata->next_slab)
            slab_metadata->next_slab->prev_slab = slab_metadata;

        slab_cache->free_slab = slab_metadata;
    }

    slab_metadata->used_count--;

    if (slab_metadata->used_count != 0)
    {
        return;
    }

    // Restore data to buddy allocated (all objects are now free)
    uint8_t buddy_order = log2_u32(slab_metadata->num_pages);
    uint8_t buddy_expon = buddy_order + BUDDY_EXPON_MIN;
    uintptr_t pages_total_size = 1 << buddy_expon; 
    uintptr_t slab_start_addr =
        (uintptr_t)slab_metadata->free_node & ~(pages_total_size-1);

    uintptr_t bytes = slab_metadata->num_pages * PAGE_SIZE;
    uintptr_t slab_tail_va = (uintptr_t)PAGE_SIZE;

    while (slab_tail_va < bytes)
    {
        phys_page_descriptor_t* tail_desc = virt_to_pfn((void*)(slab_tail_va + slab_start_addr));

        tail_desc->flags |= PAGEFLAG_BUDDY;
        tail_desc->u.slab_head = NULL;

        slab_tail_va += PAGE_SIZE;
    }

    desc->flags |= PAGEFLAG_BUDDY;
    desc->u.buddy.num_pages = desc->u.slab.num_pages;
    desc->u.buddy.buddy_cache = &heap.free_buddies[buddy_order];

    // remove from list
    if (desc->u.slab.prev_slab)
        desc->u.slab.prev_slab->next_slab = desc->u.slab.next_slab;
    if (desc->u.slab.next_slab)
        desc->u.slab.next_slab->prev_slab = desc->u.slab.prev_slab; 
    if (slab_cache->free_slab == &desc->u.slab)
        slab_cache->free_slab = desc->u.slab.next_slab;

    free_buddy((void*)slab_start_addr);
}

// Allocate abstraction for slab
void* kalloc(uintptr_t size)
{
    if (size > (1 << SLAB_EXPON_MAX))
    {
        size = align_up_pow2(size);
        
        assert(size != 0);

        return alloc_buddy(size);
    }
 
    size = align_up_pow2(size);
    if (size < (1 << SLAB_EXPON_MIN))
    {
        size = 1 << SLAB_EXPON_MIN;
    }

    assert(log2_u32(size) <= SLAB_EXPON_MAX);
    
    assert((size & (size-1)) == 0); 

    uint8_t expon = log2_u32(size); 
    uint8_t order = expon - SLAB_EXPON_MIN;

    return alloc_slab(&heap.free_slabs[order]);
}

void *kalloc_aligned(uintptr_t alignment, uintptr_t size) 
{ 
    // the size is always the alignment
    uintptr_t alloc_size = max(alignment, size);
    return kalloc(alloc_size); 
}

void *kalloc_pages(uintptr_t pages) {
  uintptr_t size = pages * PAGE_SIZE;

  size = align_up_pow2(size);

  assert(log2_u32(size) <= BUDDY_EXPON_MAX);

  return alloc_buddy(size);
}

static inline heap_slab_order_t* get_slab_order(void* obj_addr)
{
    phys_page_descriptor_t* desc = virt_to_pfn(obj_addr);
    if ((desc->flags & PAGEFLAG_HEAD) == 0)
        desc = desc->u.slab_head;

    return desc->u.slab.order_cache;
}

static uintptr_t get_buddy_size(void* buddy_addr)
{
    phys_page_descriptor_t* desc = virt_to_pfn(buddy_addr);

    assert(desc->flags & PAGEFLAG_HEAD);
    
    uint8_t expon = desc->u.buddy.order.buddy_order + BUDDY_EXPON_MIN;
    
    return 1 << expon;
}

void* krealloc(void* addr, uintptr_t new_size)
{
    if (!addr)
    {
        return kalloc(new_size);
    }
    
    phys_page_descriptor_t* desc = virt_to_pfn(addr);
    if (desc->flags & PAGEFLAG_BUDDY)
    {
        uintptr_t old_size = get_buddy_size(addr);
        if (old_size >= new_size)
            return addr;

        // Need to reallocate
        new_size = align_up_pow2(new_size);

        void* new_addr = alloc_buddy(new_size);

        if (new_addr == NULL)
            return NULL;

        memcpy(new_addr, addr, old_size);

        free_buddy(addr);

        return new_addr;
    }
    else
    {
        heap_slab_order_t* slab_order = get_slab_order(addr);
        
        uintptr_t old_size = slab_order->obj_size;
        if (old_size >= new_size)
            return addr;

        // check if it's custom, if so can't be reallocated
        if (! (&heap.free_slabs[0] <= slab_order &&
            slab_order <= &heap.free_slabs[SLAB_INDEX_ORDER_MAX]))
        {
            return NULL;
        }

        // Need to reallocate
        new_size = align_up_pow2(new_size);
        uint8_t slab_order_index = log2_u32(new_size)-SLAB_EXPON_MIN;

        void* new_addr;
        if (slab_order_index <= SLAB_INDEX_ORDER_MAX)
        {
            new_addr= alloc_slab(&heap.free_slabs[slab_order_index]);
        }
        else
        {
            new_addr = alloc_buddy(new_size);
        }

        if (new_addr == NULL)
            return NULL;

        memcpy(new_addr, addr, old_size);

        free_slab(addr);

        return new_addr;
    }
}

void kfree(void* addr)
{
    phys_page_descriptor_t* desc = virt_to_pfn(addr);

    assert(desc->type == PAGETYPE_HEAP);

    if (desc->flags & PAGEFLAG_BUDDY)
        free_buddy(addr);
    else
        free_slab(addr);
}

heap_slab_cache_t* kcreate_slab_cache(uintptr_t obj_size, const char* slab_name)
{
    assert(obj_size >= sizeof(heap_slab_node_t));
    obj_size = align_to_n(obj_size, sizeof(void*)); 

    heap_slab_cache_t* slab_cache = kalloc(sizeof(heap_slab_cache_t));

    slab_cache->name = slab_name;
    
    slab_cache->order.free_slab = NULL;
    slab_cache->order.obj_size = obj_size;
    slab_cache->order.slab_order = SLAB_CUSTOM_ORDER;
    slab_cache->order.slab_size = pages_for_obj_size(obj_size) * PAGE_SIZE;

    return slab_cache;
}

void* kalloc_cache(heap_slab_cache_t* cache)
{
    return alloc_slab(&cache->order);
}

void kfree_slab_cache(heap_slab_cache_t* slab_cache)
{
    kfree(slab_cache);
}

void init_heap(void* heap_addr, uintptr_t max_size, uintptr_t init_size)
{
    assert(sizeof(heap_slab_node_t) <= (1 << SLAB_EXPON_MIN));

    assert((uintptr_t)heap_addr % PAGE_SIZE == 0);
    assert(init_size != 0);
    assert(init_size % PAGE_SIZE == 0);
    assert(init_size < (1u << BUDDY_EXPON_MAX));

    init_heap_vars(init_size, max_size, (uintptr_t)heap_addr);

    pfn_alloc_map_pages(
        heap_addr, 
        init_size/PAGE_SIZE,
        PAGETYPE_HEAP, 
        PAGEFLAG_VFREE|PAGEFLAG_BUDDY|PAGEFLAG_KERNEL
    );

    add_buddies((uintptr_t)heap_addr, init_size, true);
}
