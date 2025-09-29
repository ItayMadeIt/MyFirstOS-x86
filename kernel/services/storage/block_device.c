#include <stddef.h>
#include <drivers/storage.h>
#include "memory/core/virt_alloc.h"
#include "memory/heap/heap.h"
#include <kernel/memory/paging.h>
#include "utils/data_structs/flat_hashmap.h"
#include "utils/data_structs/rb_tree.h"
#include <services/storage/block_device.h>
#include <kernel/core/cpu.h>
#include <memory/phys_alloc/phys_alloc.h>
#include <string.h>

#define MBR_SECTOR_SIZE 512
#define INIT_BLOCK_BUCKETS 1
#define CACHE_MULT 4
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

static intptr_t hashmap_insert_entry(stor_device_t* device, cache_entry_t* entry)
{
    return fhashmap_insert(
        &device->cache.pin_hashmap, 
        &entry->lba,
        sizeof(entry->lba),
        entry,
        0
    );
}
static intptr_t hashmap_remove_entry(stor_device_t* device, cache_entry_t* entry)
{
    return fhashmap_delete(
        &device->cache.pin_hashmap,
        &entry->lba,
        sizeof(entry->lba)
    ).succeed ? 1 : -1;
}


static intptr_t tree_insert_entry(stor_device_t* device, cache_entry_t* entry)
{
    return rb_insert(
        &device->cache.lba_tree,
        &entry->node
    ) == NULL ? -1 : 1;
}

static intptr_t tree_remake_entry(stor_device_t* device, cache_entry_t* entry, uint64_t new_lba)
{
    rb_remove_node(&device->cache.lba_tree, &entry->node);

    reset_entry(entry);
    entry->lba = new_lba;

    intptr_t result = rb_insert(&device->cache.lba_tree, &entry->node) == NULL ? -1 : 1;
    return result;
}


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
            device->cache.buckets[i] = kvalloc_pages(
                device->cache.bucket_size / PAGE_SIZE, 
                PAGETYPE_DISK_CACHE, 
                PAGEFLAG_KERNEL | PAGEFLAG_NOEXEC
            );
        }
        device->cache.buckets_length = new_len;
    }

    // Give out a buffer
    void* result_buffer = (uint8_t*)device->cache.buckets[bucket_index] + inner_block_offs;
    device->cache.used_bytes += device->block_size;

    return result_buffer;
}

static intptr_t lba_node_cmp(const rb_node_t* a_node, const rb_node_t* b_node)
{
    const cache_entry_t* a = container_of(a_node, cache_entry_t, node);
    const cache_entry_t* b = container_of(b_node, cache_entry_t, node);
    return a->lba - b->lba;
}

static void init_block_cache(stor_device_t* device)
{
    device->cache.pin_hashmap = init_fhashmap();
    rb_init_tree(&device->cache.lba_tree, lba_node_cmp, NULL);

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

static void block_queue_insert_lru(stor_device_t* device, cache_entry_t* entry)
{
    cache_entry_t* old_lru = device->cache.cache_queue.lru;
    
    entry->q_next = NULL;
    entry->q_prev = old_lru; 

    // Old lru switches
    if (old_lru)
    {
        old_lru->q_next = entry;
    }
    // Is first element? Switch MRU
    else
    {
        device->cache.cache_queue.mru = entry;
    }

    device->cache.cache_queue.lru = entry;
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
    if (entry->q_next)
        entry->q_next->q_prev = entry->q_prev;
    // was mru
    else
        device->cache.cache_queue.mru = entry->q_prev;

    if (entry->q_prev)
        entry->q_prev->q_next = entry->q_next;
    // was lru
    else
        device->cache.cache_queue.lru = entry->q_next;
    
    entry->q_prev = NULL; 
    entry->q_next = NULL; 

    return entry;
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

static void clean_dirty(stor_device_t* device, void* buffer)
{
    for (uint32_t i = 0; i < device->pages_per_block; i++)
    {
        clear_phys_flags(buffer + PAGE_SIZE * i, VIRT_PHYS_FLAG_DIRTY);
    }
}

// checks if the block is dirty, and if so makes it clean but returns true, otherwise false
static bool is_entry_dirty(stor_device_t* device, cache_entry_t* entry)
{
    bool reset_dirty = false;

    for (uint32_t i = 0; i < device->pages_per_block; i++)
    {
        if (reset_dirty)
        {
            clear_phys_flags(entry->buffer + PAGE_SIZE * i, VIRT_PHYS_FLAG_DIRTY);
            continue;
        }

        uint32_t flags = get_phys_flags(entry->buffer + PAGE_SIZE * i);
        if (flags & VIRT_PHYS_FLAG_DIRTY)
        {   
            reset_dirty = true;
            clear_phys_flags(entry->buffer + PAGE_SIZE * i, VIRT_PHYS_FLAG_DIRTY);
        }
    }
    
    return reset_dirty | entry->dirty;
}

static void flush_stor_entry_sync(stor_device_t* device, cache_entry_t* entry)
{
    // Maybe change it to scan next LBA to write into
    bool dirty = is_entry_dirty(device, entry);
    if (!dirty)
        return;

    stor_request_chunk_entry_t chunk = {
        .va_buffer = entry->buffer,
        .sectors = device->block_size/device->sector_size,
    };

    bool done = false;
    stor_request_t request = {
        .action = STOR_REQ_WRITE,
        .dev = device,
        
        .lba = entry->lba,
        .chunk_list = &chunk,
        .chunk_length = 1,
        
        .callback = sync_set_done,
        .user_data = &done,
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
        entry = entry->q_next;
    }
}

void stor_mark_dirty(cache_entry_t * entry)
{
    entry->dirty = true;
}

void stor_flush_all(stor_device_t* device)
{
    stor_iterate_queue(device, flush_stor_entry_sync);
}

static cache_entry_t* evict_cache_entry_sync(stor_device_t* device)
{
    cache_entry_t* entry = block_queue_evict(device);
    if (!entry)
    {
        return NULL;
    }

    flush_stor_entry_sync(device, entry);
    entry->cur_ref_count = 1;
    entry->lba_next = NULL;

    flat_hashmap_result_t result = fhashmap_delete(
        &device->cache.pin_hashmap, 
        &entry->lba, 
        sizeof(entry->lba)
    );

    if (result.succeed)
    {
        return NULL;
    }
    
    return entry;
}

static cache_entry_t* create_cache_entry(stor_device_t* device, uint64_t cache_lba, void* buffer)
{
    cache_entry_t* entry = kalloc(sizeof(cache_entry_t));

    reset_entry(entry);

    entry->lba = cache_lba;
    entry->buffer = buffer;

    tree_insert_entry(device, entry);

    return entry;
}

static cache_entry_t* create_speculative_entry(stor_device_t* device, uint64_t lba) 
{
    // already cached
    if (fhashmap_get_data(&device->cache.pin_hashmap, &lba, sizeof(lba)).succeed) 
    {
        return NULL; 
    }

    void* buffer = block_request_buffer(device);
    cache_entry_t* entry;
    if (!buffer) 
    {
        entry = evict_cache_entry_sync(device);
        assert(entry);
        clean_dirty(device, entry->buffer);
        
        tree_remake_entry(device, entry, lba);
    }
    else 
    {
        entry = create_cache_entry(device, lba, buffer);
    }

    hashmap_insert_entry(device, entry);

    entry->cur_ref_count = 0; // speculative
    block_queue_insert_lru(device, entry);

    return entry;
}







static cache_entry_t* find_or_repin_entry(stor_device_t* dev, uint64_t lba) 
{
    flat_hashmap_result_t result = fhashmap_get_data(
        &dev->cache.pin_hashmap, 
        &lba, 
        sizeof(lba)
    );

    if (!result.succeed) 
    {
        return NULL;
    }

    cache_entry_t* entry = result.value;

    if (entry->cur_ref_count == 0) 
    {
        entry->cur_ref_count = 1;
        block_queue_pop(dev, entry);
    } 
    else 
    {
        entry->cur_ref_count++;
    }

    return entry;
}
static cache_entry_t* alloc_or_evict_entry(stor_device_t* device, uint64_t lba) 
{
    void* buffer = block_request_buffer(device);
    cache_entry_t* entry;

    if (!buffer) 
    {
        entry = evict_cache_entry_sync(device);
        assert(entry);
        clean_dirty(device, entry->buffer);

        reset_entry(entry);

        entry->lba = lba;
        tree_insert_entry(device, entry);
    } 
    else 
    {
        entry = create_cache_entry(device, lba, buffer);
    }

    entry->cur_ref_count = 1;

    hashmap_insert_entry(device, entry);
    
    return entry;
}










static void fetch_entry_sync(stor_device_t* device, cache_entry_t* entry, uint64_t cache_lba)
{
    stor_request_chunk_entry_t chunk_entry = {
        .va_buffer = entry->buffer,
        .sectors = device->block_size/device->sector_size,
    };

    bool done = false;
    stor_request_t request = {
        .action = STOR_REQ_READ,
        .callback = sync_set_done,
        .dev = device,
        .lba = cache_block_to_lba(device, cache_lba),
        .user_data = &done,
        .chunk_list = &chunk_entry,
        .chunk_length = 1,
    };
    device->submit(&request);

    while (! done)
    {
        cpu_halt();
    }
}

static void fetch_entries_sync(stor_device_t* device, cache_entry_t* first, 
                                uint64_t cache_lba, const uint64_t count)
{
    stor_request_chunk_entry_t chunk_entries[count];
    cache_entry_t* entry = first;
    for (uint64_t i = 0; i < count; i++)
    {
        assert(entry);

        chunk_entries[i] = (stor_request_chunk_entry_t) {
            .va_buffer = entry->buffer,
            .sectors   = device->block_size/device->sector_size,
        };

        entry = entry->lba_next;
    }

    bool done = false;
    stor_request_t request = {
        .dev = device,
        .action = STOR_REQ_READ,
        .lba = cache_block_to_lba(device, cache_lba),
        
        .callback = sync_set_done,
        .user_data = &done,

        .chunk_list = chunk_entries,
        .chunk_length = count,
    };
    device->submit(&request);

    while (! done)
    {
        cpu_halt();
    }
}

cache_entry_t* stor_pin_sync(stor_device_t* device, uint64_t cache_lba)
{
    flat_hashmap_result_t result = fhashmap_get_data(
        &device->cache.pin_hashmap, 
        &cache_lba, 
        sizeof(cache_lba)
    );

    // if found
    if (result.succeed)
    {
        cache_entry_t* entry = result.value; 
        // Was evicted (not in queue)
        if (entry->cur_ref_count == 0)
        {
            entry->cur_ref_count = 1;
            block_queue_remove_entry(device, entry);
        }
        // In queue
        else
        {
            entry->cur_ref_count++;
        }

        return entry;
    }

    void* buffer = block_request_buffer(device);
    
    cache_entry_t* entry;
 
    // No buffer == has to evict
    if (buffer == NULL)
    {
        entry = evict_cache_entry_sync(device);
    }
    // create entry
    else
    {
        entry = create_cache_entry(device, cache_lba, buffer);
    }
    (void)buffer;
    
    clean_dirty(device, entry->buffer);

    entry->lba = cache_lba;
    entry->cur_ref_count = 1;
    hashmap_insert_entry(device, entry);
    
    fetch_entry_sync(device, entry, cache_lba);

    return entry;
}

void stor_unpin_sync(stor_device_t* device, cache_entry_t* cache_entry)
{
    assert(cache_entry);
    assert(cache_entry->cur_ref_count > 0);

    cache_entry->cur_ref_count--;
    if (cache_entry->cur_ref_count == 0)
    {
        hashmap_remove_entry(device, cache_entry);
        block_queue_insert_mru(device, cache_entry);
    }
}

cache_entry_t* stor_pin_range_sync(stor_device_t* device, uint64_t cache_lba, uint64_t count)
{
    cache_entry_t* first = NULL;
    cache_entry_t* prev  = NULL;

    cache_entry_t* first_fetch = NULL;
    uint64_t fetch_count = 0;

    // Gather all the cache blocks
    for (uint64_t i = 0; i < count; i++)
    {
        uint64_t cur_lba = cache_lba + i;

        cache_entry_t* entry = find_or_repin_entry(device, cur_lba);

        // Existing entry
        if (entry) 
        {
            if (prev) 
                prev->lba_next = entry;
            prev = entry;

            if (!first) 
                first = entry;
            
            if (fetch_count > 0) 
            {
                fetch_entries_sync(device, first_fetch, first_fetch->lba, fetch_count);
                first_fetch = NULL;
                fetch_count = 0;
            }
        } 
        // New entry (allocate or evict)
        else 
        {
            // (currently evict one by one instead of chunks)
            entry = alloc_or_evict_entry(device, cur_lba);

            if (!first_fetch) 
                first_fetch = entry;

            if (prev) 
                prev->lba_next = entry;
            prev = entry;

            if (!first) 
                first = entry;

            fetch_count++;
        }
    }
    if (prev) 
        prev->lba_next = NULL;


    // Handle last fetch / read ahead blocks
    if (fetch_count > 0) 
    {
        // Handle read ahead
        uint64_t read_ahead_blocks = min(
            MAX_READAHEAD_BLOCKS, 
            device->max_blocks - (first_fetch->lba + fetch_count)
        );

        uint64_t ra_count = 0;

        while (ra_count < read_ahead_blocks)
        {
            uint64_t ra_lba = first_fetch->lba + fetch_count + ra_count;

            flat_hashmap_result_t result = fhashmap_get_data(
                &device->cache.pin_hashmap,
                &ra_lba,
                sizeof(ra_lba)
            );

            // ra_lba exists, stop fetch count
            if (result.succeed)
                break;

            cache_entry_t* spec_entry = create_speculative_entry(device, ra_lba);
            ra_count++;
            if (prev)
                prev->lba_next = spec_entry;
            prev = spec_entry;
        }

        fetch_entries_sync(device, first_fetch, first_fetch->lba, fetch_count + ra_count);
    }
    
    return first;
}

void stor_unpin_range_sync(stor_device_t* device, cache_entry_t* first_entry, uint64_t count)
{
    cache_entry_t* entry_it = first_entry;
    for (size_t i = 0; i < count; i++)
    {
        assert(entry_it);
        assert(entry_it->cur_ref_count > 0);

        entry_it->cur_ref_count--;
        if (entry_it->cur_ref_count == 0)
        {
            hashmap_remove_entry(device, entry_it);
            block_queue_insert_mru(device, entry_it);
        }

        entry_it = entry_it->lba_next;
    }
}
// TEST TEST & TEST, first test that the sync layer is perfect, only then do shit
// TEST TEST & TEST, first test that the sync layer is perfect, only then do shit
// TEST TEST & TEST, first test that the sync layer is perfect, only then do shit
// TEST TEST & TEST, first test that the sync layer is perfect, only then do shit
// TEST TEST & TEST, first test that the sync layer is perfect, only then do shit
// TEST TEST & TEST, first test that the sync layer is perfect, only then do shit
// TEST TEST & TEST, first test that the sync layer is perfect, only then do shit

void init_block_device(stor_device_t* device)
{   
    init_block_cache(device);
}