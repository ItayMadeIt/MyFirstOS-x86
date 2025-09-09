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

void* alloc_and_map_pages(uintptr_t count, uint16_t page_flags);
bool remap_pages(void* va_ptr, uintptr_t count, uint16_t new_flags);

#endif // __VIRT_ALLOC_H__