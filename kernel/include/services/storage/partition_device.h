#ifndef __PARTITION_DEVICE_H__
#define __PARTITION_DEVICE_H__

#include "block_device.h"
#include "partition.h"

void stor_part_pin_range_sync(
    stor_partition_t* partition,
    usize block_lba,
    usize count,
    cache_entry_t** out_entries);

void stor_part_unpin_range_sync(
    stor_partition_t* partition,
    cache_entry_t** arr,
    usize count);


void stor_part_pin_range_async(
    stor_partition_t* partition,
    usize block_lba,
    usize count,
    cache_entry_t** out_entries, 
    stor_pin_range_cb_t cb,
    void* ctx);

void stor_part_unpin_range_async(
    stor_partition_t* partition,
    cache_entry_t** arr,
    usize count,
    stor_unpin_range_cb_t cb,
    void* ctx);

#endif // PARTITION_DEVICE_h