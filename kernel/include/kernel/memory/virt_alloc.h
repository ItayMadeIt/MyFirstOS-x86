#ifndef __VIRT_ALLOC_H__
#define __VIRT_ALLOC_H__

#include <core/defs.h>
#include <memory/core/pfn_desc.h>

bool map_pages(void* va_ptr, uintptr_t count, enum phys_page_type page_type, uint16_t page_flags);
bool map_phys_pages(void* pa_ptr, void* va_ptr, uintptr_t count, enum phys_page_type page_type, uint16_t page_flags);
void* identity_map_pages(void* pa_ptr, uintptr_t count, enum phys_page_type page_type, uint16_t page_flags);

bool map_page(void* va, enum phys_page_type page_type, uint16_t page_flags);
bool map_phys_page(void* pa_ptr, void* va_ptr, enum phys_page_type page_type, uint16_t page_flags);
bool unmap_pages(void* va_ptr, uintptr_t count);

void* get_phys_addr(void* va);

typedef enum virt_phys_flags
{
    VIRT_PHYS_FLAG_PRESENT  = 1 << 0,
    VIRT_PHYS_FLAG_DIRTY    = 1 << 1,
    VIRT_PHYS_FLAG_ACCESS   = 1 << 2,
    VIRT_PHYS_FLAG_USER     = 1 << 3,
    VIRT_PHYS_FLAG_WRITE    = 1 << 4,
    VIRT_PHYS_FLAG_NOEXEC   = 1 << 5,

} virt_phys_flags_t;

uint32_t get_phys_flags(void* va);
void clear_phys_flags(void* va, uint32_t flags);

void* alloc_pages(uintptr_t count, uint32_t type, uint32_t page_flags);
bool remap_pages(void* va_ptr, uintptr_t count, uint32_t new_flags);
void free_pages(void* va_ptr);

#endif // __VIRT_ALLOC_H__