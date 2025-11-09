#include "services/storage/block_range.h"
#include "drivers/storage.h"
#include "memory/heap/heap.h"
#include "services/storage/block_device.h"


typedef struct stor_block_range 
{
    usize     begin_block_lba;
    usize     lba_block_count;
    usize_ptr offset_in_begin;
} stor_block_range_t;

static stor_block_range_t range_calc_block(stor_device_t* dev, usize offset, usize size)
{
    stor_block_range_t range;
    range.begin_block_lba = offset / dev->cache.block_size;
    usize last_block = (offset + size - 1) / dev->cache.block_size;
    range.lba_block_count = last_block - range.begin_block_lba + 1;
    range.offset_in_begin = offset % dev->cache.block_size;
    return range;
}



stor_range_mapping_t stor_kvrange_map_sync(stor_device_t* dev, usize offset, usize size)
{
    stor_block_range_t range = range_calc_block(dev, offset, size);

    cache_entry_t** entries = kalloc(sizeof(cache_entry_t*) * range.lba_block_count);
    stor_pin_range_sync(dev, range.begin_block_lba, range.lba_block_count, entries);

    void* clone = stor_clone_kvrange_entries(dev, entries, range.lba_block_count);

    return (stor_range_mapping_t){
        .entries = entries,
        .block_count = range.lba_block_count,
        .va = clone,
        .offset_in_first = range.offset_in_begin
    };
}

void stor_kvrange_unmap_sync(stor_device_t* dev, stor_range_mapping_t* m)
{
    stor_free_kvrange(m->va);
    stor_unpin_range_sync(dev, m->entries, m->block_count);
    kfree(m->entries);
}




typedef struct kvrange_map_async_ctx
{
    stor_device_t* dev;
    cache_entry_t** entries;
    stor_block_range_t range;
    
    stor_kvrange_map_cb_t cb;
    void* ctx;
} kvrange_map_async_ctx_t;


static void map_range_pin_done_cb(void* ctx, int status, cache_entry_t** entries, u64 count)
{
    kvrange_map_async_ctx_t* meta = ctx;

    if (status < 0) 
    {
        if (meta->cb) 
        {
            meta->cb(meta->ctx, status, NULL);
        }
        kfree(meta);
        return;
    }

    void* clone = stor_clone_kvrange_entries(meta->dev, entries, count);

    stor_range_mapping_t mapping = {
        .entries = entries,
        .block_count = count,
        .va = clone,
        .offset_in_first = meta->range.offset_in_begin,
    };

    if (meta->cb)
    {
        meta->cb(meta->ctx, 0, &mapping);
    }

    kfree(meta);
}


void stor_kvrange_map_async(
    stor_device_t* dev,
    usize offset,
    usize size,
    stor_kvrange_map_cb_t cb,
    void* ctx)
{
    stor_block_range_t range = range_calc_block(dev, offset, size);
    cache_entry_t** entries = kalloc(sizeof(cache_entry_t*) * range.lba_block_count);

    kvrange_map_async_ctx_t* meta = kalloc(sizeof(kvrange_map_async_ctx_t));
    *meta = (kvrange_map_async_ctx_t){
        .dev = dev,
        .entries = entries,
        .range = range,
        
        .cb = cb,
        .ctx = ctx,
    };

    stor_pin_range_async(
        dev,
        range.begin_block_lba,
        range.lba_block_count,
        entries,
        map_range_pin_done_cb,
        meta
    );
}





typedef struct kvrange_unmap_async_ctx
{
    stor_kvrange_unmap_cb_t cb;
    void* ctx;
    stor_range_mapping_t* mapping;
} kvrange_unmap_async_ctx_t;

static void unmap_done_cb(void* ctx, int status)
{
    kvrange_unmap_async_ctx_t* meta = ctx;
    
    kfree(meta->mapping->entries);

    if (meta->cb)
    {
        meta->cb(meta->ctx, status);
    }
    
    kfree(meta);
}

void stor_kvrange_unmap_async(
    stor_device_t* dev, 
    stor_range_mapping_t* m,
    stor_kvrange_unmap_cb_t cb,
    void* ctx)
{
    stor_free_kvrange(m->va);

    kvrange_unmap_async_ctx_t* meta = kalloc(sizeof(kvrange_unmap_async_ctx_t));
    *meta = (kvrange_unmap_async_ctx_t){ 
        .cb = cb, 
        .ctx = ctx, 
        .mapping = m 
    };

    stor_unpin_range_async(
        dev, 
        m->entries, 
        m->block_count, 
        unmap_done_cb, 
        meta
    );
}