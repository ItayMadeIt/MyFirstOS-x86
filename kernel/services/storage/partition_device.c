#include <services/storage/partition_device.h>

void stor_part_pin_range_async(stor_partition_t *partition, uint64_t block_lba, uint64_t count, cache_entry_t **out_entries, stor_pin_range_cb_t cb, void *ctx)
{
    return stor_pin_range_async(
        partition->device,
        partition->block_lba + block_lba,
        count, 
        out_entries, 
        cb, ctx 
    );
}

void stor_part_unpin_range_async(
    stor_partition_t* partition,
    cache_entry_t** arr,
    uint64_t count,
    stor_unpin_range_cb_t cb,
    void* ctx)
{
    return stor_unpin_range_async(
        partition->device,
        arr,
        count,
        cb,
        ctx
    );
}

void stor_part_pin_range_sync(
    stor_partition_t* partition,
    uint64_t block_lba,
    uint64_t count,
    cache_entry_t** out_entries)
{
    return stor_pin_range_sync(
        partition->device,
        partition->block_lba + block_lba,
        count,
        out_entries
    );
}

void stor_part_unpin_range_sync(
    stor_partition_t* partition,
    cache_entry_t** arr,
    uint64_t count)
{
    return stor_unpin_range_sync(
        partition->device,
        arr,
        count
    );
}
