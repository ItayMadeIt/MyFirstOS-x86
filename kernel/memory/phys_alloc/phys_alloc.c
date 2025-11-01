#include <memory/phys_alloc/phys_alloc.h>

void* (*alloc_phys_page)();
phys_alloc_t (*alloc_phys_pages)(const usize_ptr count);
void (*free_phys_page)(void* pa);
void (*free_phys_pages)(phys_alloc_t free_params);
