#include <core/defs.h>
#include <core/paging.h>
#include <memory/page_frame.h>

bool map_pages(void* va, uint32_t count, uint32_t flags, enum phys_page_type type);