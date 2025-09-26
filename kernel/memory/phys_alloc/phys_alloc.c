#include <memory/phys_alloc/phys_alloc.h>

void* (*alloc_phys_page)(enum phys_page_type type, uint16_t flags);
phys_alloc_t (*alloc_phys_pages)(const uintptr_t count, enum phys_page_type type, uint16_t flags);
void (*free_phys_page)(void* pa);
void (*free_phys_pages)(phys_alloc_t free_params);
