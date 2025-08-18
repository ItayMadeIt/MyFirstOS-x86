#ifndef __MEMORY_MANAGER_H__
#define __MEMORY_MANAGER_H__

#include <memory/phys_alloc.h>
#include <memory/page_frame.h>
#include <core/defs.h>

extern void* (*alloc_phys_page)(enum phys_page_type type, uint32_t flags);
void setup_memory(multiboot_info_t* mbd);

#endif // __MEMORY_MANAGER_H__