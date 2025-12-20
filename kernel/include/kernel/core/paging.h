#ifndef __PAGING_H__
#define __PAGING_H__

// arch could change
#include <arch/i386/core/paging.h>
#include <arch/i386/memory/paging_utils.h>

#define round_page_down(addr)  ((usize_ptr)(addr) & PAGE_MASK)
#define round_page_up(addr)    (((usize_ptr)(addr) + PAGE_SIZE - 1) & PAGE_MASK)

#define page_index(addr) ((usize_ptr)(addr) / PAGE_SIZE)

#endif // __PAGING_H__