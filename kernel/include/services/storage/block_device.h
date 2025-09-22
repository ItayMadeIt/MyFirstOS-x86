#ifndef __BLOCK_DEVICE_H__
#define __BLOCK_DEVICE_H__

#include "utils/data_structs/flat_hashmap.h"
#include <stdint.h>
typedef struct stor_device stor_device_t;

void* stor_get_sync(stor_device_t* device, uint64_t cache_lba);
void stor_flush_all(stor_device_t* device);

typedef struct cache_entry 
{
    uint64_t lba;
    void* buffer;
    
    struct cache_entry* next;
    struct cache_entry* prev;

} cache_entry_t;

typedef struct cache_queue
{
    // [mru] -> next -> next -> [lru]
    // [mru] <- prev <- prev <- [lru]

    cache_entry_t* mru; // most recently used
    cache_entry_t* lru; // least recently used
} cache_queue_t;

typedef struct block_device_data 
{
    flat_hashmap_t hashmap;

    void** buckets;            // array of bucket base pointers
    uint64_t buckets_length;   // number of allocated buckets
    uint64_t bucket_size;      // cache_size * constant

    uint64_t max_bytes;        // max amount of bytes for the entire buckets
    uint64_t used_bytes;       // track current usage

    cache_queue_t cache_queue; // Queue
} block_device_data_t;

void init_block_device(stor_device_t* device);

#endif // __BLOCK_DEVICE_H__