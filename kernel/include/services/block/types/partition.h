#ifndef __BLOCK_PARTITION_H__
#define __BLOCK_PARTITION_H__

#include "core/num_defs.h"

typedef struct block_device block_device_t;

typedef struct block_dev_partition
{
    block_device_t* disk_block_device;
    usize bytes_offset;

} block_dev_partition_t;

block_device_t* block_partition_generate(
    block_device_t* disk_block_device, 
    usize lba_offset, usize sector_count);

#endif // __BLOCK_PARTITION_H__