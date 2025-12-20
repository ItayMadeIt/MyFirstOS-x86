#ifndef __BLOCK_MANAGER_H__
#define __BLOCK_MANAGER_H__

#include "services/block/device.h"

void block_manager_add_device(block_device_t* device);
block_device_t* block_manager_get_device(usize_ptr index);
block_device_t* block_manager_main_device();

void init_block_manager();

#endif // __BLOCK_MANAGER_H__