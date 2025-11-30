#ifndef __BLOCK_TYPES_H__
#define __BLOCK_TYPES_H__

#include "services/block/types/disk.h"
#include "services/block/types/partition.h"

enum block_dev_type
{
    BLOCK_DEV_DISK,
    BLOCK_DEV_PARTITION,
    // BLOCK_DEV_FILE, (will be loopback) 
};

union block_dev_data
{
    block_dev_disk_t disk;
    block_dev_partition_t partition;
};

#endif // __BLOCK_TYPES_H__