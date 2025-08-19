#include <core/defs.h>
#include <core/paging.h>
#include <memory/page_frame.h>

bool map_pages(void* va_ptr, uint32_t count, enum phys_page_type page_type, uint32_t page_flags);