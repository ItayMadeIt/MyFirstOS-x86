#include <services/storage/partition_device.h>

void stor_part_pin_range_async(stor_partition_t *partition, u64 block_lba, u64 count, cache_entry_t **out_entries, stor_pin_range_cb_t cb, void *ctx)
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
    u64 count,
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
    u64 block_lba,
    u64 count,
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
    u64 count)
{
    return stor_unpin_range_sync(
        partition->device,
        arr,
        count
    );
}
