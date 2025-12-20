#include "arch/i386/memory/paging_utils.h"
#include "memory/core/pfn_desc.h"
#include "memory/virt/virt_region.h"
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
    u16 vflags;

} virt_interval_t;

const char* vregion_to_str(enum virt_region_type vregion)
{
    switch (vregion) {
        case VREGION_HEAP:
            return "HEAP";
        case VREGION_PFN:
            return "PFN";
        case VREGION_ACPI:
            return "ACPI";
        case VREGION_DRIVER:
            return "DRIVER";
        case VREGION_USER:
            return "USER";
        case VREGION_RESREVED:
            return "RESERVED";
        case VREGION_CACHE:
            return "CACHE";
        case VREGION_BIO_BUFFER:
            return "BIO BUFFER";
        case VREGION_MMIO:
            return "MMIO";
        case VREGION_KERNELIMG:
            return "KERNEL_IMG";
        default:
            abort();
    }
    return NULL;
}

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

void kvregion_mark(void* from, usize_ptr count, enum virt_region_type vregion, const char* name)
{
    virt_interval_t* new_interval = kalloc_cache(interval_cache);
    assert(new_interval);
    
    new_interval->from = (usize_ptr)from;
    new_interval->to   = (usize_ptr)from + count * PAGE_SIZE;
    new_interval->name = name;
    new_interval->vregion = vregion;

    rb_insert(&virt_kernel_tree, &new_interval->node);
}

void kvregion_release(void* va)
{
    virt_interval_t* cur_interval = search_interval_addr(
        &virt_kernel_tree,
        va
    );
    assert(cur_interval);

    rb_remove_node(
        &virt_kernel_tree, 
        &cur_interval->node
    );
}

void* kvregion_reserve(usize_ptr count, enum virt_region_type vregion, const char* name)
{
    usize_ptr size = count * PAGE_SIZE;

    void* gap_start = find_gap_start(&virt_kernel_tree, size, HIGH_VADDR, MAX_VADDR);
    if (!gap_start)
    {
        return NULL;
    }

    kvregion_mark(
        gap_start,  count, 
        vregion, name ? name : RESERVE_PAGES_NAME
    );

    return gap_start;
}

bool kvregion_is_free(void* va)
{
    if (!va)
    {
        return false;
    }

    assert(((usize_ptr)va & (PAGE_SIZE - 1)) == 0);

    virt_interval_t probe = {
        .from = (usize_ptr)va,
        .to   = (usize_ptr)va + PAGE_SIZE
    };

    rb_node_t* node = rb_search(&virt_kernel_tree, &probe.node);

    return (node == NULL);
}

usize_ptr  kvregion_count(void* va)
{
    virt_interval_t probe = {
        .from = (usize_ptr)va,
        .to   = (usize_ptr)va + PAGE_SIZE
    };

    rb_node_t* node = rb_search(&virt_kernel_tree, &probe.node);

    virt_interval_t* interval = container_of(node, virt_interval_t, node);

    return (interval->to - interval->from);
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


void init_virt_region()
{
    rb_init_tree(&virt_kernel_tree, interval_cmp, NULL);
    
    interval_cache = kcreate_slab_cache(
        sizeof(virt_interval_t), 
        "Interval Memory Regions"
    );
}