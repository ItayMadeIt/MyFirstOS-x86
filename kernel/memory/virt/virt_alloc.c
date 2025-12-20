#include "memory/virt/virt_alloc.h"
#include "memory/core/memory_manager.h"
#include "memory/virt/virt_region.h"
#include "memory/virt/virt_map.h"

void* kvalloc_pages(
    usize_ptr count, 
    enum virt_region_type vregion)
{
    void* va = kvregion_reserve(count, vregion, vregion_to_str(vregion));

    page_t* page = mm_alloc_pages(count);
    vmap_pages(va, page, vregion, count);

    return va;
}

void  kvfree_pages(void* va_ptr)
{
    kvregion_release(va_ptr);
}