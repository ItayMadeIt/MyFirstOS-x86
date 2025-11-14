#include "memory/core/pfn_desc.h"
#include <memory/core/virt_alloc.h>
#include <core/defs.h>
#include <memory/heap/heap.h>
#include <stddef.h>
#include "core/num_defs.h"
#include <utils/data_structs/rb_tree.h>
#include <kernel/memory/paging.h>

#define ALLOC_PAGES_NAME "Virtual Allocation"
#define RESERVE_PAGES_NAME "Virtual Reserved"

static rb_tree_t virt_kernel_tree;
static heap_slab_cache_t* interval_cache;

typedef struct virt_interval
{
    rb_node_t node;

    usize_ptr from;
    usize_ptr to;
    
    const char* name;

    enum virt_region_type vregion;

} virt_interval_t;

static void* find_gap_start(rb_tree_t* tree, usize_ptr size, usize_ptr min_addr, usize_ptr max_inclusive_addr)
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
    if (!next_node)
    {
        if ((max_inclusive_addr - cur->to + 1) >= size)
        {
            return (void*)cur->to;
        }
        else
        {
            return NULL;
        }
    }
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
        .from = (usize_ptr)va_ptr,
        .to   = (usize_ptr)va_ptr + PAGE_SIZE
    };

    rb_node_t* node = rb_search(tree, &asked_interval.node);
    if (node == NULL)
    {
        return NULL;
    }

    virt_interval_t* interval = container_of(node, virt_interval_t, node);
    return interval;
}

void kvmark_region(void* from, void* to, enum virt_region_type vregion, const char* name)
{
    virt_interval_t* new_interval = kalloc_cache(interval_cache);
    assert(new_interval);
    
    new_interval->from = (usize_ptr)from;
    new_interval->to   = (usize_ptr)to;
    new_interval->name = name;
    new_interval->vregion = vregion;

    rb_insert(&virt_kernel_tree, &new_interval->node);
}

void kvunmark_region(void* from, void* to)
{
    virt_interval_t* cur_interval = search_interval_addr(
        &virt_kernel_tree,
        from
    );
    assert(cur_interval && cur_interval->to == (usize_ptr) to);

    rb_remove_node(
        &virt_kernel_tree, 
        &cur_interval->node
    );
}

void* kvreserve_pages(usize_ptr count, enum virt_region_type vregion, const char* name)
{
    usize_ptr size = count * PAGE_SIZE;

    void* gap_start = find_gap_start(&virt_kernel_tree, size, HIGH_VADDR, MAX_VADDR);
    if (!gap_start)
    {
        return NULL;
    }

    kvmark_region(
        gap_start,  (u8*)gap_start + size, 
        vregion, name ? name : RESERVE_PAGES_NAME
    );

    return gap_start;
}

void kvunreserve_pages(void* va)
{
    virt_interval_t* interval = search_interval_addr(&virt_kernel_tree, va);
    if (!interval)
    {
        abort();
    } 

    rb_remove_node(&virt_kernel_tree, &interval->node);

    kfree(interval);
}

void* kvalloc_pages(usize_ptr count, enum virt_region_type vregion, u16 page_flags)
{
    void* result = kvreserve_pages(count, vregion, ALLOC_PAGES_NAME);

    pfn_alloc_map_pages(
        result, count, 
        virt_to_phys_type(vregion), page_flags
    );

    return result;
}

void kvfree_pages(void* va)
{
    virt_interval_t* interval = search_interval_addr(&virt_kernel_tree, va);
    unmap_pages(va, (interval->to - interval->from) / PAGE_SIZE);

    kvunreserve_pages(va);
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

void* kvmap_phys_vec(const phys_run_vec_t* run, enum virt_region_type vregion, u16 page_flags)
{
    usize_ptr total_pages = 0;
    for (u64 i = 0; i < run->run_count; i++) 
    {
        total_pages += run->runs[i].count;
    }

    void* start = kvreserve_pages(total_pages, vregion, ALLOC_PAGES_NAME);

    usize_ptr cur_va_gap = (usize_ptr) start ;

    for (u64 i = 0; i < run->run_count; i++) 
    {
        pfn_map_pages(
            run->runs[i].addr, (void*)cur_va_gap, 
            run->runs[i].count, 
            virt_to_phys_type(vregion), page_flags
        );

        cur_va_gap += PAGE_SIZE * run->runs[i].count;
    }
    
    return start;
}

void kvunmap_phys_vec(void* va, const phys_run_vec_t* run)
{
    assert(va);
    assert(run && run->run_count > 0);

    pfn_unref_vrange(va, run->total_pages);
    unmap_pages(va, run->total_pages);

    kvunmark_region(va, (u8*)va + run->total_pages * PAGE_SIZE);
}

void* vmap_identity(void* pa, usize_ptr count, enum virt_region_type type, u16 flags)
{
    pfn_identity_map_pages(pa, count, virt_to_phys_type(type), flags);

    return pa;
}

void init_virt_alloc()
{
    rb_init_tree(&virt_kernel_tree, interval_cmp, NULL);
    
    interval_cache = kcreate_slab_cache(
        sizeof(virt_interval_t), 
        "Interval Memory Regions"
    );
}