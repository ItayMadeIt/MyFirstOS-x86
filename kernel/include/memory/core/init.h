#ifndef __MEMORY_INIT_H__
#define __MEMORY_INIT_H__

#include "kernel/boot/boot_data.h"

extern usize_ptr max_memory;

extern usize_ptr kernel_begin_pa;
extern usize_ptr kernel_end_pa;
extern usize_ptr kernel_size;

void init_memory(boot_data_t* mbd);

#endif // __MEMORY_INIT_H__