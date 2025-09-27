#include <memory/core/virt_alloc.h>
#include <core/defs.h>
#include <memory/heap/heap.h>
#include <string.h>
#include <utils/data_structs/rb_tree.h>
#include <kernel/memory/paging.h>
#include <stdio.h>

#define ALLOC_PAGES_NAME "Virtual Allocation"

static rb_tree_t virt_kernel_tree;
static heap_slab_cache_t* interval_cache;

typedef struct virt_interval
{
    rb_node_t node;

    uintptr_t from;
    uintptr_t to;
    
    const char* name;

} virt_interval_t;

static void* find_gap_start(rb_tree_t* tree, uintptr_t size, uintptr_t min_addr, uintptr_t max_inclusive_addr)
{
    rb_node_t* cur_node = rb_min(tree);
    if (cur_node == NULL) 
    {
        if ((max_inclusive_addr - min_addr + 1) >= size) 
        {
            return (void*)min_addr;
        }
        return NULL;
    }
    virt_interval_t* cur = container_of(cur_node, virt_interval_t, node);


    assert(cur->from >= min_addr);

    // Check if there is an interval from min_addr to start
    if (cur->from - min_addr >= size)
    {
        return (void*)min_addr;
    }

    // Check if there is a free interval between something
    rb_node_t* next_node = rb_next(cur_node);
    virt_interval_t* next = container_of(next_node, virt_interval_t, node);
    while (next_node != NULL)
    {
        cur  = container_of(cur_node , virt_interval_t, node);
        next = container_of(next_node, virt_interval_t, node);

        if (next->from - cur->to >= size)
        {
            return (void*)cur->to;
        }

        cur_node = next_node;
        next_node = rb_next(cur_node);
    }

    // Free interval between end and max_addr
    if ((max_inclusive_addr - cur->to) + 1 >= size)
    {
        return (void*)cur->to;
    }

    return NULL;
}

static inline virt_interval_t* search_interval_addr(rb_tree_t* tree, void* va_ptr)
{
    virt_interval_t asked_interval = {
        .from = (uintptr_t)va_ptr,
        .to   = (uintptr_t)va_ptr + PAGE_SIZE
    };

    rb_node_t* node = rb_search(tree, &asked_interval.node);
    if (node == NULL)
    {
        return NULL;
    }

    virt_interval_t* interval = container_of(node, virt_interval_t, node);
    return interval;
}

void* kvalloc_pages(uintptr_t count, uint16_t page_type, uint16_t page_flags)
{
    uintptr_t size = count * PAGE_SIZE;

    void* gap_start = find_gap_start(&virt_kernel_tree, size, HIGH_VADDR, MAX_VADDR);

    if (! map_pages(gap_start, count, page_type, page_flags))
    {
        return NULL;
    }

    virt_mark_region(gap_start, gap_start + size, ALLOC_PAGES_NAME);

    return gap_start;
}

void kvfree_pages(void* va_ptr)
{
    virt_interval_t* interval = search_interval_addr(&virt_kernel_tree, va_ptr);
    if (!interval)
    {
        abort();
    } 

    unmap_pages(va_ptr, (interval->to - interval->from) / PAGE_SIZE);

    rb_remove_node(&virt_kernel_tree, &interval->node);

    kfree(interval);
}

static int interval_cmp(const rb_node_t* node_a, const rb_node_t* node_b)
{
    const virt_interval_t* int_a = container_of(node_a, virt_interval_t, node);
    const virt_interval_t* int_b = container_of(node_b, virt_interval_t, node);

    if (int_a->to <= int_b->from) 
        return -1;

    if (int_b->to <= int_a->from) 
        return 1;
    
    return 0;
}

void init_virt_alloc()
{
    rb_init_tree(&virt_kernel_tree, interval_cmp, NULL);
    
    interval_cache = kcreate_slab_cache(
        sizeof(virt_interval_t), 
        "Interval Memory Regions"
    );
}

void virt_mark_region(void* from, void* to, const char* name)
{
    virt_interval_t* new_interval = kalloc_cache(interval_cache);
    assert(new_interval);

    new_interval->from = (uintptr_t)from;
    new_interval->to   = (uintptr_t)to;
    new_interval->name = name;

    rb_insert(&virt_kernel_tree, &new_interval->node);
}