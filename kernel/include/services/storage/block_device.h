#ifndef __BLOCK_DEVICE_H__
#define __BLOCK_DEVICE_H__

#include "utils/data_structs/flat_hashmap.h"
#include "utils/data_structs/rb_tree.h"
#include <stdint.h>
typedef struct stor_device stor_device_t;
typedef struct cache_entry cache_entry_t;

cache_entry_t* stor_pin_sync(stor_device_t* device, uint64_t cache_lba);
void stor_unpin_sync(stor_device_t* device, cache_entry_t* cache_entry);

cache_entry_t* stor_pin_range_sync(stor_device_t* device, uint64_t cache_lba, uint64_t count);
void stor_unpin_range_sync(stor_device_t* device, cache_entry_t* first_entry, uint64_t count);

void stor_mark_dirty(cache_entry_t* entry);

void stor_flush_all(stor_device_t* device);

typedef struct cache_entry 
{
    // data
    uint64_t lba;
    void* buffer;
    uint32_t cur_ref_count;
    
    bool dirty;
    
    // tree
    rb_node_t node;

    union {
        // queue (if ref count == 0)
        struct {
            struct cache_entry* q_next;
            struct cache_entry* q_prev;
        };
        // lba + 1 entry
        struct cache_entry* lba_next;
    };

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
    flat_hashmap_t pin_hashmap;
    rb_tree_t lba_tree;
    cache_queue_t cache_queue; // Queue

    void** buckets;            // array of bucket base pointers
    uint64_t buckets_length;   // number of allocated buckets
    uint64_t bucket_size;      // cache_size * constant

    uint64_t max_bytes;        // max amount of bytes for the entire buckets
    uint64_t used_bytes;       // track current usage

} block_device_data_t;


void init_block_device(stor_device_t* device);

#endif // __BLOCK_DEVICE_H__