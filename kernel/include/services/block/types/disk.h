#ifndef __BLOCK_DISK_H__
#define __BLOCK_DISK_H__

#include "drivers/storage.h"
#include "utils/data_structs/ring_queue.h"

typedef struct block_device block_device_t;

typedef struct block_dev_disk
{
    stor_device_t* hw_device;
    ring_queue_t queue;

} block_dev_disk_t;

block_device_t* block_disk_generate(stor_device_t* stor_dev);

#endif // __BLOCK_DISK_H__