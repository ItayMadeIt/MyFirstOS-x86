#ifndef __BLOCK_RANGE_H__
#define __BLOCK_RANGE_H__

#include "services/storage/block_device.h"

usize calc_blocks_per_bytes(stor_device_t *device, usize offset, usize amount);

typedef struct stor_range_mapping
{
    cache_entry_t** entries;
    usize block_count;
    void* va;
    usize offset_in_first;
} stor_range_mapping_t;

stor_range_mapping_t stor_kvrange_map_sync(stor_device_t* dev, usize offset, usize size);
void stor_kvrange_unmap_sync(stor_device_t* dev, stor_range_mapping_t* m);

typedef void (*stor_kvrange_map_cb_t)(
    void* ctx,
    int status,
    stor_range_mapping_t* mapping
);

typedef void (*stor_kvrange_unmap_cb_t)(
    void* ctx,
    int status
);

void stor_kvrange_map_async(
    stor_device_t* dev,
    usize offset,
    usize size,
    stor_kvrange_map_cb_t cb,
    void* ctx);

void stor_kvrange_unmap_async(
    stor_device_t* dev, 
    stor_range_mapping_t* m,
    stor_kvrange_unmap_cb_t cb,
    void* ctx
);

#endif // __BLOCK_RANGE_H__