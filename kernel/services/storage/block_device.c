#include <core/atomic_defs.h>
#include <services/storage/block_device.h>
#include <stdatomic.h>
#include <stddef.h>
#include <drivers/storage.h>
#include "core/defs.h"
#include "memory/core/virt_alloc.h"
#include "memory/heap/heap.h"
#include <kernel/memory/paging.h>
#include "utils/data_structs/flat_hashmap.h"
#include "utils/data_structs/rb_tree.h"
#include <kernel/core/cpu.h>
#include <memory/phys_alloc/phys_alloc.h>
#include "core/num_defs.h"
#include <stdio.h>
#include <string.h>

#include "kernel/interrupts/irq.h"

#define MBR_SECTOR_SIZE 512
#define INIT_BLOCK_BUCKETS 1
#define CACHE_MULT 4 // has to be 2^n
#define MAX_BYTES_DIVIDOR 4

#define MAX_READAHEAD_BLOCKS 2

/*
Here’s a richer state machine that covers the lifecycle:

UNUSED - freshly allocated, no data, not mapped into the tree/hash yet.

Transition: allocated -> fetch starts -> READING.

READING - async read in progress, buffer being filled.

Transition: callback -> AVAILABLE.

AVAILABLE - valid data, matches storage, safe to use/evict.

Transition: written to -> DIRTY.

DIRTY - has changes not yet persisted.

Transition: write scheduled → WRITING.

WRITING - async write in progress, can’t evict.

Transition: callback -> CLEAN.

EVICTING - entry chosen for eviction, but waiting for in-flight I/O to finish.

Transition: once I/O completes -> UNUSED.
*/

static void reset_entry(cache_entry_t* entry)
{
    assert(entry);
    entry->buffer = NULL;
    entry->cur_ref_count = 0;
    entry->lba = 0;

    entry->dirty = false;

    memset(&entry->node, 0, sizeof(entry->node));

    entry->lba_next = NULL;

    entry->q_prev = NULL;
    entry->q_next = NULL;
}

static ssize_ptr hashmap_pin_insert_entry(stor_device_t* device, cache_entry_t* entry)
{
    return fhashmap_insert(
        &device->cache.pin_hashmap, 
        &entry->lba,
        sizeof(entry->lba),
        entry,
        0
    );
}

static cache_entry_t* hashmap_pin_get_entry(stor_device_t* device, usize block_lba)
{
    cache_entry_t* return_value = NULL;

    flat_hashmap_result_t hashmap_result = fhashmap_get_data(
        &device->cache.pin_hashmap, &block_lba, sizeof(block_lba)
    );

    if (hashmap_result.succeed)
        return_value = hashmap_result.value;

    return return_value;
}

static bool hashmap_pin_del_entry(stor_device_t* device, usize block_lba)
{
    flat_hashmap_result_t hashmap_result = fhashmap_delete(
        &device->cache.pin_hashmap, &block_lba, sizeof(block_lba)
    );

    return hashmap_result.succeed;
}

static ssize_ptr tree_insert_entry(stor_device_t* device, cache_entry_t* entry)
{
    return rb_insert(
        &device->cache.lba_tree,
        &entry->node
    ) == NULL ? -1 : 1;
}

static ssize_ptr tree_remake_entry(stor_device_t* device, cache_entry_t* entry, usize new_lba)
{
    rb_remove_node(&device->cache.lba_tree, &entry->node);

    entry->lba = new_lba;

    ssize_ptr result = rb_insert(&device->cache.lba_tree, &entry->node) == NULL ? -1 : 1;
    return result;
}


static void* block_request_buffer(stor_device_t* device)
{   
    assert(device);

    if (device->cache.used_bytes + device->cache.block_size > device->cache.max_bytes)
        return NULL;

    usize block_index     = device->cache.used_bytes / device->cache.block_size;

    // Create more blocks
    if (block_index >= device->cache.blocks_length)
    {
        usize old_len = device->cache.blocks_length;
        usize new_len = old_len ? old_len * 2 : 4;

        device->cache.blocks_arr = krealloc(
            device->cache.blocks_arr,
            new_len * sizeof(void*)
        );
        assert(device->cache.blocks_arr);

        // Allocate blocks
        for (usize i = old_len; i < new_len; i++)
        {
            void* block_va = kvalloc_pages(
                device->cache.pages_per_block,
                VREGION_CACHE,
                PAGEFLAG_KERNEL | PAGEFLAG_NOEXEC
            );
            assert(block_va);
            device->cache.blocks_arr[i] = block_va;
        }

        device->cache.blocks_length = new_len;
    }

    // Give out a buffer
    void* result_buffer = (u8*)device->cache.blocks_arr[block_index];
    device->cache.used_bytes += device->cache.block_size;

    return result_buffer;
}

static ssize_ptr lba_node_cmp(const rb_node_t* a_node, const rb_node_t* b_node)
{
    const cache_entry_t* a = container_of(a_node, cache_entry_t, node);
    const cache_entry_t* b = container_of(b_node, cache_entry_t, node);
    
    if (a->lba < b->lba) 
        return -1;
    
    if (a->lba > b->lba) 
        return  1;
    
    return 0;
}

static void init_block_cache(stor_device_t* device)
{
    device->cache.pin_hashmap = init_fhashmap();
    rb_init_tree(&device->cache.lba_tree, lba_node_cmp, NULL);

    device->cache.max_bytes = max_memory / MAX_BYTES_DIVIDOR;
    // (dummy limit for debug) : device->cache.max_bytes = device->block_size * CACHE_MULT;
    device->cache.used_bytes = 0;

    device->cache.blocks_arr = kalloc(sizeof(void*) * INIT_BLOCK_BUCKETS);
    device->cache.blocks_length = INIT_BLOCK_BUCKETS;

    device->cache.cache_queue.mru = NULL;
    device->cache.cache_queue.lru = NULL;

    // max buckets_length == device->cache.max_bytes / device->cache.bucket_size

    for (usize_ptr i = 0; i < device->cache.blocks_length; i++)
    {
        device->cache.blocks_arr[i] = kvalloc_pages (
            device->cache.pages_per_block, 
            VREGION_CACHE, 
            PAGEFLAG_KERNEL | PAGEFLAG_NOEXEC
        );
    }    
}

static void block_queue_insert_mru(stor_device_t* device, cache_entry_t* entry)
{
    cache_entry_t* old_mru = device->cache.cache_queue.mru;
    
    entry->q_prev = NULL;
    entry->q_next = old_mru; 

    // Old mru switches
    if (old_mru)
    {
        old_mru->q_prev = entry;
    }
    // Is first element? Switch LRU
    else
    {
        device->cache.cache_queue.lru = entry;
    }

    device->cache.cache_queue.mru = entry;
}

static void block_queue_remove_entry(stor_device_t* device, cache_entry_t* entry)
{
    if (!entry)
    {
        return;
    }

    // Fix prev link
    if  (entry->q_prev)
    {
        entry->q_prev->q_next = entry->q_next;
    }
    // Was MRU
    else
    {
        device->cache.cache_queue.mru = entry->q_next;
    }

    // Fix next link
    if (entry->q_next)
    {
        entry->q_next->q_prev = entry->q_prev;
    }
    // Was LRU
    else
    {
        device->cache.cache_queue.lru = entry->q_prev;
    }

    entry->q_prev = NULL;
    entry->q_next = NULL;
}

static cache_entry_t* block_queue_pop(stor_device_t* device, cache_entry_t* entry)
{
    cache_queue_t* q = &device->cache.cache_queue;

    if (entry->q_next)
        entry->q_next->q_prev = entry->q_prev;
    // was lru
    else
        q->lru = entry->q_prev;

    if (entry->q_prev)
        entry->q_prev->q_next = entry->q_next;
    // was mru
    else
        q->mru = entry->q_next;
    
    entry->q_prev = NULL; 
    entry->q_next = NULL; 

    return entry;
}

static void block_queue_for_each(stor_device_t* device, void (*callback)(cache_entry_t*, void*), void* ctx)
{
    assert(device);
    assert(callback);

    cache_entry_t* current = device->cache.cache_queue.mru; // start from MRU
    while (current)
    {
        callback(current, ctx);
        cache_entry_t* next = current->q_next;
        current = next;
    }
}

static cache_entry_t* block_queue_evict(stor_device_t* device)
{
    if (!device->cache.cache_queue.lru)
        return NULL;

    return block_queue_pop(device, device->cache.cache_queue.lru);
}

static void stor_clear_dirty(stor_device_t* device, cache_entry_t* entry)
{
    entry->dirty = false;

    // Goes over pages dirty (will be removed in the near future)
    for (u32 i = 0; i < device->cache.pages_per_block; i++)
    {
        clear_phys_flags(entry->buffer + PAGE_SIZE * i, VIRT_PHYS_FLAG_DIRTY);
    }
}

// checks if the block is dirty, and if so makes it clean but returns true, otherwise false
static bool consume_cache_dirty(stor_device_t* device, cache_entry_t* entry)
{
    bool reset_dirty = false;

    // Goes over pages dirty (will be removed in the near future)
    for (u32 i = 0; i < device->cache.pages_per_block; i++)
    {
        if (reset_dirty)
        {
            clear_phys_flags(entry->buffer + PAGE_SIZE * i, VIRT_PHYS_FLAG_DIRTY);
            continue;
        }

        u32 flags = get_phys_flags(entry->buffer + PAGE_SIZE * i);
        if (flags & VIRT_PHYS_FLAG_DIRTY)
        {
            reset_dirty = true;
            clear_phys_flags(entry->buffer + PAGE_SIZE * i, VIRT_PHYS_FLAG_DIRTY);
        }
    }
    
    bool result = entry->dirty;

    entry->dirty = false;

    return result;
}

void stor_mark_dirty(cache_entry_t *entry)
{
    entry->dirty = true;

    uptr irq = irq_save();

    if (entry->cur_ref_count && entry->state == CACHE_STATE_CLEAN)
    {
        entry->state = CACHE_STATE_DIRTY;
    }

    irq_restore(irq);
}

static cache_entry_t *create_cache_entry(stor_device_t *device,
                                         usize block_lba, void *buffer,
                                         enum cache_state state, u32 init_ref_count)
{
    cache_entry_t* entry = kalloc(sizeof(cache_entry_t));

    reset_entry(entry);

    entry->lba    = block_lba;
    entry->buffer = buffer;
    entry->state  = state;
    entry->cur_ref_count = init_ref_count;
    entry->dirty = false;

    tree_insert_entry(device, entry);

    return entry;
}

static cache_entry_t* tree_get_entry(stor_device_t* device, usize lba)
{
    cache_entry_t key = {.lba=lba};
    rb_node_t* result_node = rb_search(&device->cache.lba_tree, &key.node);

    if (result_node)
        return container_of(result_node, cache_entry_t, node);
    else
        return NULL;
}

static void unpin_entry(stor_device_t* device, usize block_lba)
{
    cache_entry_t* entry = hashmap_pin_get_entry(device, block_lba);
    assert(entry);

    entry->cur_ref_count--;
    if (entry->cur_ref_count > 0)
        return;

    hashmap_pin_del_entry(device, entry->lba);
    block_queue_insert_mru(device, entry);
}

typedef struct pin_range 
{
    usize block_lba;
    usize block_count;
    
    usize remaining_to_evict;

    usize range_index;
} pin_range_t;

typedef struct pin_range_fn_metadata
{
    stor_device_t* device;
    usize block_lba;
    usize block_count;
    usize fix_ranges_count;
    usize total_evict_count; // all evict count

    stor_pin_range_cb_t cb;
    void* ctx;

    cache_entry_t** entries;

    usize ranges_count;
    pin_range_t ranges[];


} pin_range_fn_metadata_t;

static void read_range_cb_async(stor_request_t* request, int64_t status)
{
    assert(status>=0);

    pin_range_t* range = request->user_data;
    for (usize i = 0; i < range->block_count; i++)
    {
        cache_entry_t* entry = hashmap_pin_get_entry(request->dev, range->block_lba + i);
        assert(entry);
        
        if (entry->dirty) // shouldn't happen
            entry->state = CACHE_STATE_DIRTY;
        else
            entry->state = CACHE_STATE_CLEAN;
    }

    pin_range_fn_metadata_t* metadata = container_of(range, pin_range_fn_metadata_t, ranges[range->range_index]);

    metadata->fix_ranges_count--;
    if (metadata->fix_ranges_count == 0)
    {
        if (metadata->cb)
        {
            metadata->cb(metadata->ctx, 0, metadata->entries, metadata->block_count);
        }
        kfree(metadata);
    }
}

static void call_read_range_async(stor_device_t* device, pin_range_t* range)
{
    stor_request_chunk_entry_t chunks[range->block_count];
    for (usize i = 0; i < range->block_count; i++)
    {
        cache_entry_t* entry = hashmap_pin_get_entry(device, range->block_lba + i);

        assert(entry);

        entry->state = CACHE_STATE_READING;

        chunks[i].va_buffer = entry->buffer;
        chunks[i].sectors   = device->cache.block_size / device->sector_size;
    }

    stor_request_t request = {
        .action = STOR_REQ_READ,
        .callback = read_range_cb_async,
        .dev = device,

        .chunk_list = chunks,
        .chunk_length = range->block_count,

        .lba = cache_block_to_lba(device, range->block_lba),
        .user_data = range,
    };

    device->submit(&request);
}

static void write_entry_cb_async(stor_request_t* request, int64_t status)
{
    assert(status>=0);
    
    cache_entry_t* entry = request->user_data;
    pin_range_t* range = entry->io.own_range;
    pin_range_fn_metadata_t* metadata = container_of(range, pin_range_fn_metadata_t, ranges[range->range_index]);

    // Rebanrd evicted entry
    metadata->entries[entry->io.new_index] = entry;
    tree_remake_entry(metadata->device, entry, entry->io.new_lba);
    hashmap_pin_insert_entry(metadata->device, entry);
    entry->cur_ref_count = 1;

    if (entry->dirty)
        entry->state = CACHE_STATE_DIRTY;
    else
        entry->state = CACHE_STATE_CLEAN;

    // Check if range is finished
    range->remaining_to_evict--;
    if (range->remaining_to_evict == 0)
    {
        call_read_range_async(metadata->device, range);
    }
}

static void call_evict_async(stor_device_t* device, pin_range_t* range, usize index_in_range)
{
    pin_range_fn_metadata_t* metadata = container_of(range, pin_range_fn_metadata_t, ranges[range->range_index]);

    usize begin_range_entry_index = range->block_lba - metadata->block_lba;
    usize entry_index = begin_range_entry_index + index_in_range;
    usize entry_lba   = range->block_lba + index_in_range;

    // Evict entry
    assert(metadata->entries[entry_index] == NULL);
    
    cache_entry_t* evict_entry = block_queue_evict(device);
    usize evict_lba   = evict_entry->lba;

    assert(evict_entry);

    // Rebrand it to the new entry
    evict_entry->io.new_lba = entry_lba;
    evict_entry->io.new_index = entry_index;
    evict_entry->io.own_range = range;

    // No need to flush
    if (! consume_cache_dirty(device, evict_entry))
    {
        range->remaining_to_evict--;
        if (range->remaining_to_evict == 0)
        {
            call_read_range_async(metadata->device, range);
        }
        return;
    }

    // Flush entry and continue
    stor_clear_dirty(device, evict_entry);

    evict_entry->state = CACHE_STATE_FLUSHING;

    stor_request_chunk_entry_t chunk = {
        .va_buffer = evict_entry->buffer,
        .sectors = device->cache.block_size / device->sector_size
    };
    stor_request_t request = {
        .action = STOR_REQ_WRITE,
        .callback = write_entry_cb_async,
        .dev = device,

        .chunk_list = &chunk,
        .chunk_length = 1,

        .lba = cache_block_to_lba(device, evict_lba),
        .user_data = evict_entry,
    };
    device->submit(&request);

}

void stor_pin_range_async(
    stor_device_t * device, usize block_lba, 
    usize count, cache_entry_t ** out_cache_arr, 
    stor_pin_range_cb_t cb, void* ctx)
{
    uptr irq = irq_save();

    pin_range_fn_metadata_t* metadata = 
        kalloc(sizeof(pin_range_fn_metadata_t) + sizeof(pin_range_t) * count);

    metadata->block_lba = block_lba;
    metadata->block_count = count;
    metadata->device = device;
    
    metadata->fix_ranges_count = 0;
    metadata->ranges_count = 0;
    metadata->total_evict_count = 0;

    metadata->cb = cb;
    metadata->ctx = ctx;
    metadata->entries = out_cache_arr;

    usize range_begin_index = 0;
    bool is_in_range = false;
    
    for (usize i = 0; i < count; i++)
    {
        // Add to pinned 
        cache_entry_t* entry = hashmap_pin_get_entry(device, block_lba + i);
        if (entry)
        {
            entry->cur_ref_count++;
            out_cache_arr[i] = entry;

            if (is_in_range)
            {
                usize block_count = i - range_begin_index; // not including this entry
                metadata->ranges[metadata->ranges_count].block_count = block_count;
                metadata->ranges[metadata->ranges_count].remaining_to_evict = block_count;
                metadata->total_evict_count += block_count;
                metadata->ranges_count++;
            }
            is_in_range = false;

            continue;
        }

        // If unpinned but allocated, re-pin it
        entry = tree_get_entry(device, block_lba + i);
        if (entry)
        {
            block_queue_remove_entry(device, entry);
            hashmap_pin_insert_entry(device, entry);
            entry->cur_ref_count = 1;
            out_cache_arr[i] = entry;

            if (is_in_range)
            {
                usize block_count = i - range_begin_index; // not including this entry
                metadata->ranges[metadata->ranges_count].block_count = block_count;
                metadata->ranges[metadata->ranges_count].remaining_to_evict = block_count;
                metadata->total_evict_count += block_count;
                metadata->ranges_count++;
            }
            is_in_range = false;

            continue;
        }

        // 2 expensive options from now on:
        //
        // 1. Allocate buffers and read data
        // 2. First evict/write to disk buffers & then use them to read data
        // 
        // Will just mark the range in the ranges value in metadata (will be used in the second loop) 
        if (is_in_range)
        {
            out_cache_arr[i] = NULL;
            continue;
        }

        range_begin_index = i;
        is_in_range = true;

        out_cache_arr[i] = NULL;

        metadata->ranges[metadata->ranges_count].range_index = metadata->ranges_count;
        metadata->ranges[metadata->ranges_count].block_lba = block_lba + i;

        metadata->ranges[metadata->ranges_count].remaining_to_evict = ~0; // not known yet
    }
    
    if (is_in_range)
    {
        usize block_count = count - range_begin_index;
        metadata->ranges[metadata->ranges_count].block_count = block_count;
        metadata->ranges[metadata->ranges_count].remaining_to_evict = block_count;

        metadata->total_evict_count += metadata->ranges[ metadata->ranges_count ].remaining_to_evict;
        metadata->ranges_count++;
        is_in_range = false;
    }
    
    metadata->fix_ranges_count = metadata->ranges_count;

    // For each range, call async handle (handle eviction if neccesary & batch read)
    for (usize i = 0; i < metadata->ranges_count; i++)
    {
        pin_range_t* range = &metadata->ranges[i];

        bool did_evict = false;

        usize range_arr_offset = range->block_lba - block_lba;

        for (usize j = 0; j < range->block_count; j++)
        {
            void* block_buffer = block_request_buffer(device);
            if (block_buffer)
            {
                cache_entry_t* entry = create_cache_entry(
                    device, 
                    range->block_lba + j, 
                    block_buffer, 
                    CACHE_STATE_UNUSED, 1
                );
                out_cache_arr[range_arr_offset + j] = entry;
                hashmap_pin_insert_entry(device, entry);
                range->remaining_to_evict--;
            }
            else
            {
                did_evict = true;

                call_evict_async(device, range, j);
            }
        }

        if (! did_evict)
        {
            // call async READ
            // which will: metadata->fix_ranges_count--;
            // if fix_ranges_count == 0, it's done, call callback
            call_read_range_async(device, range);
        }
    }

    irq_restore(irq);
}

void stor_unpin_range_async(
    stor_device_t* device,
    cache_entry_t** arr,
    usize count,
    stor_unpin_range_cb_t cb,
    void* ctx)
{
    for (usize i = 0; i < count; i++)
    {
        unpin_entry(device, arr[i]->lba);
        arr[i] = NULL;
    }
    
    if (cb)
    {
        cb(ctx, 1);   
    }
}

static void mark_pin_range_done(void* data, int status, cache_entry_t** entries, usize count)
{
    (void)status;(void)entries;(void)count;

    bool* done = data;
    *done = true;
}

void stor_pin_range_sync(stor_device_t *device, usize block_lba, usize count, cache_entry_t **out_entries)
{
    bool done = false;

    stor_pin_range_async(
        device, 
        block_lba, 
        count,
        out_entries, 
        mark_pin_range_done, 
        &done    
    );

    while (!done)
    {
        cpu_halt();
    }
} 

static void mark_unpin_range_done(void* data, int status)
{
    (void)status;

    bool* done = data;
    *done = true;
}
void stor_unpin_range_sync(stor_device_t* device, cache_entry_t** arr,  usize count)
{
    bool done = false;

    stor_unpin_range_async(
        device, 
        arr, 
        count,
        mark_unpin_range_done, 
        &done    
    );

    while (!done)
    {
        cpu_halt();
    }
}
static void flush_entry_callback(stor_request_t* request, i64 status)
{
    (void)request;
    cache_entry_t* entry = request->user_data;
    assert(status >= 0 && entry);

    if (entry->dirty)
        entry->state = CACHE_STATE_DIRTY;
    else
        entry->state = CACHE_STATE_CLEAN;
}
static void flush_entry(cache_entry_t* entry, void* ctx)
{
    stor_device_t* device = ctx;

    // No need to flush
    if (! consume_cache_dirty(device, entry))
    {
        return;
    }

    // Flush entry and continue
    stor_clear_dirty(device, entry);

    entry->state = CACHE_STATE_FLUSHING;

    stor_request_chunk_entry_t chunk = {
        .va_buffer = entry->buffer,
        .sectors = device->cache.block_size / device->sector_size
    };
    stor_request_t request = {
        .action = STOR_REQ_WRITE,
        .callback = flush_entry_callback,
        .dev = device,

        .chunk_list = &chunk,
        .chunk_length = 1,

        .lba = cache_block_to_lba(device, entry->lba),
        .user_data = entry,
    };
    device->submit(&request);
}

void stor_flush_unpinned(stor_device_t *device)
{
    uptr irq = irq_save();

    block_queue_for_each(device, flush_entry, device);

    irq_restore(irq);
}

usize calc_blocks_per_bytes(stor_device_t *device, usize offset, usize amount)
{
    return (amount + device->cache.block_size - 1) / device->cache.block_size
        + (offset % device->cache.block_size != 0 ? 1 : 0);
}

void* block_dev_vbuffer(cache_entry_t* entry)
{
    return entry->buffer;
}

usize_ptr block_dev_bufsize(stor_device_t* device)
{
    return device->cache.block_size; 
}

void* stor_clone_vrange(stor_device_t* device, cache_entry_t** entries, usize count)
{
    usize total_bytes = count * device->cache.block_size;
    usize total_pages = (total_bytes + PAGE_SIZE - 1) / PAGE_SIZE;

    void* clone_base = kvalloc_pages(
        total_pages,
        VREGION_CACHE_CLONE,
        PAGEFLAG_KERNEL | PAGEFLAG_NOEXEC
    );
    assert(clone_base);

    u8* dst_cursor = clone_base;

    for (usize i = 0; i < count; i++) 
    {
        void* src_va = entries[i]->buffer;
        pfn_clone_map(dst_cursor, src_va, device->cache.pages_per_block);
        dst_cursor += device->cache.block_size;
    }

    return clone_base;
}


void init_block_device(stor_device_t* device)
{   
    init_block_cache(device);
}