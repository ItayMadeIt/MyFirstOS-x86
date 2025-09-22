#include <stddef.h>
#include <drivers/storage.h>
#include "memory/heap/heap.h"
#include <kernel/memory/virt_alloc.h>
#include "utils/data_structs/flat_hashmap.h"
#include <services/storage/block_device.h>
#include <kernel/core/cpu.h>
#include <string.h>

#define MBR_SECTOR_SIZE 512
#define INIT_BLOCK_BUCKETS 1
#define CACHE_MULT 4
#define MAX_BYTES_DIVIDOR 4

static void* block_request_buffer(stor_device_t* device)
{   
    assert(device->cache.used_bytes + device->block_size <= device->cache.max_bytes);

    uint64_t bucket_index     = device->cache.used_bytes / device->cache.bucket_size;
    uint64_t inner_block_offs = device->cache.used_bytes % device->cache.bucket_size;

    // Create more buckets
    if (bucket_index >= device->cache.buckets_length)
    {
        size_t new_len = device->cache.buckets_length * 2;
        device->cache.buckets = krealloc(
            device->cache.buckets,
            new_len * sizeof(void*)
        );
        assert(device->cache.buckets);

        for (size_t i = device->cache.buckets_length; i < new_len; i++)
        {
            device->cache.buckets[i] = kalloc(device->cache.bucket_size);
        }
        device->cache.buckets_length = new_len;
    }

    // Give out a buffer
    void* result_buffer = (uint8_t*)device->cache.buckets[bucket_index] + inner_block_offs;
    device->cache.used_bytes += device->block_size;

    return result_buffer;
}

extern uintptr_t max_memory;

static void init_block_cache(stor_device_t* device)
{
    device->cache.hashmap = init_fhashmap();

    device->cache.max_bytes = max_memory / MAX_BYTES_DIVIDOR;
    device->cache.used_bytes = 0;

    device->cache.buckets = kalloc(sizeof(void*) * INIT_BLOCK_BUCKETS);
    device->cache.buckets_length = INIT_BLOCK_BUCKETS;
    device->cache.bucket_size = CACHE_MULT * device->block_size;

    device->cache.cache_queue.mru = NULL;
    device->cache.cache_queue.lru = NULL;

    // max buckets_length == device->cache.max_bytes / device->cache.bucket_size

    for (uintptr_t i = 0; i < device->cache.buckets_length; i++)
    {
        device->cache.buckets[i] = kalloc(device->cache.bucket_size);
    }    
}

static void block_queue_insert(stor_device_t* device, cache_entry_t* entry)
{
    cache_entry_t* last_mru = device->cache.cache_queue.mru;
    
    entry->next = NULL;
    entry->prev = last_mru; 

    // Old mru switches
    if (last_mru)
    {
        last_mru->next = entry;
    }
    // Is first element? Switch LRU
    else
    {
        device->cache.cache_queue.lru = entry;
    }

    device->cache.cache_queue.mru = entry;
}

static cache_entry_t* block_queue_pop(stor_device_t* device, cache_entry_t* entry)
{
    if (entry->next)
        entry->next->prev = entry->prev;
    // was mru
    else
        device->cache.cache_queue.mru = entry->prev;

    if (entry->prev)
        entry->prev->next = entry->next;
    // was lru
    else
        device->cache.cache_queue.lru = entry->next;
    
    entry->prev = NULL; 
    entry->next = NULL; 

    return entry;
}

static void block_queue_promote(stor_device_t* device, cache_entry_t* entry)
{
    if (device->cache.cache_queue.mru == entry)
        return;

    block_queue_pop(device, entry);
    block_queue_insert(device, entry);
}

static cache_entry_t* block_queue_evict(stor_device_t* device)
{
    if (!device->cache.cache_queue.lru)
        return NULL;

    return block_queue_pop(device, device->cache.cache_queue.lru);
}

static void sync_set_done(stor_request_t* request, int64_t result)
{
    (void)request; (void)result;

    bool* done = request->user_data;
    *done = true;
}

static void flush_dirty(stor_device_t* device, void* buffer)
{
    for (uint32_t i = 0; i < device->pages_per_block; i++)
    {
        clear_phys_flags(buffer + PAGE_SIZE * i, VIRT_PHYS_FLAG_DIRTY);
    }
}

// checks if the block is dirty, and if so makes it clean but returns true, otherwise false
static bool is_block_dirty(stor_device_t* device, void* buffer)
{
    bool reset_dirty = false;

    for (uint32_t i = 0; i < device->pages_per_block; i++)
    {
        if (reset_dirty)
        {
            clear_phys_flags(buffer + PAGE_SIZE * i, VIRT_PHYS_FLAG_DIRTY);
            continue;
        }

        uint32_t flags = get_phys_flags(buffer + PAGE_SIZE * i);
        if (flags & VIRT_PHYS_FLAG_DIRTY)
        {   
            reset_dirty = true;
            clear_phys_flags(buffer + PAGE_SIZE * i, VIRT_PHYS_FLAG_DIRTY);
        }
    }
    
    return reset_dirty;
}

static void flush_stor_entry_sync(stor_device_t* device, cache_entry_t* entry)
{
    bool dirty = is_block_dirty(device, entry->buffer);
    if (!dirty)
    {
        return;
    }

    bool done = false;
    stor_request_t request = {
        .action = STOR_REQ_WRITE,
        .callback = sync_set_done,
        .dev = device,
        .lba = entry->lba,
        .sectors = device->block_size/device->sector_size,
        .user_data = &done,
        .va_buffer = entry->buffer,
    };

    device->submit(&request);
    while(!done)
    {
        cpu_halt();
    }
}

static void stor_iterate_queue(
    stor_device_t* device, void (*callback)(stor_device_t* device, cache_entry_t* entry))
{
    cache_entry_t* entry = device->cache.cache_queue.mru;
    while (entry)
    {
        callback(device, entry);
        entry = entry->next;
    }
}

void stor_flush_all(stor_device_t* device)
{
    stor_iterate_queue(device, flush_stor_entry_sync);
}

void* stor_get_sync(stor_device_t* device, uint64_t cache_lba)
{
    flat_hashmap_result_t result = fhashmap_get_data(
        &device->cache.hashmap, 
        &cache_lba, 
        sizeof(cache_lba)
    );

    // if found
    if (result.succeed)
    {
        cache_entry_t* entry = result.value; 

        block_queue_promote(device, entry);

        return entry->buffer;
    }

    void* buffer = block_request_buffer(device);
    
    cache_entry_t* entry;

    if (buffer == NULL)
    {
        entry = block_queue_evict(device);

        flush_stor_entry_sync(device, entry);

        // cache blocks are full, thus queue has at least 1 entry
        assert(entry);

        flat_hashmap_result_t result = fhashmap_delete(
            &device->cache.hashmap, 
            &entry->lba, 
            sizeof(entry->lba)
        );

        assert(result.succeed);

        buffer = entry->buffer;
    }
    else
    {
        entry = kalloc(sizeof(cache_entry_t));
        entry->next = NULL;
        entry->prev = NULL;

        entry->buffer = buffer;

        block_queue_insert(device, entry);
    }

    entry->lba = cache_lba;

    fhashmap_insert(
        &device->cache.hashmap, 
        &entry->lba, 
        sizeof(entry->lba), 
        entry, 
        0
    );

    bool done = false;
    stor_request_t request = {
        .action = STOR_REQ_READ,
        .callback = sync_set_done,
        .dev = device,
        .lba = cache_block_to_lba(device, cache_lba),
        .sectors = device->cache.bucket_size / device->block_size,
        .user_data = &done,
        .va_buffer = buffer,
    };
    device->submit(&request);

    while (! done)
    {
        cpu_halt();
    }

    flush_dirty(device, entry->buffer);

    return buffer;
}

void init_block_device(stor_device_t* device)
{   
    init_block_cache(device);
}