#ifndef __BLOCK_DEVICE_H__
#define __BLOCK_DEVICE_H__

#include "utils/data_structs/flat_hashmap.h"
#include "utils/data_structs/rb_tree.h"
#include <stdint.h>
typedef struct stor_device stor_device_t;
typedef struct cache_entry cache_entry_t;

enum cache_state {
    CACHE_STATE_INVALID    = 0, // invalid 
    CACHE_STATE_READING    = 1, // I/O operation, waiting for read to finish
    CACHE_STATE_CLEAN      = 2, // clean (pin/unpin)
    CACHE_STATE_DIRTY      = 3, // dirty (pin/unpin)
    CACHE_STATE_FLUSHING   = 4, // I/O operation, waiting for write to finish 
    CACHE_STATE_UNUSED     = 5, // not used by anything, ref count == 0, waiting to be acquired (AFTER FLUSHING)
    /*
    State      | Event                     | Next State
    ___________|___________________________|____________
    UNUSED     | acquire                   | READING
    READING    | read ahead complete       | CLEAN # REF_COUNT=0
    READING    | read complete             | CLEAN # REF_COUNT=1
    CLEAN      | acquire                   | CLEAN # REF_COUNT++
    CLEAN      | write (marked dirty)      | DIRTY
    CLEAN      | eviction                  | UNUSED
    DIRTY      | acquire                   | DIRTY
    DIRTY      | eviction (REF_COUNT=0)    | FLUSHING 
    FLUSHING   | flush complete            | UNUSED 
    */
};

typedef struct cache_entry 
{
    // data
    uint64_t lba;
    void* buffer;
    uint32_t cur_ref_count;
    uint8_t  state;
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

    uint64_t max_blocks;

    uint64_t block_size; // max(page_size, sector_size)
    uint64_t pages_per_block; 

} block_device_data_t;

typedef void (*stor_pin_cb_t)(void* ctx, int status, cache_entry_t* entry);
typedef void (*stor_unpin_cb_t)(void* ctx, int status);

typedef void (*stor_pin_range_cb_t)(void* ctx, int status, cache_entry_t** entries, uint64_t count);
typedef void (*stor_unpin_range_cb_t)(void* ctx, int status);


// void stor_pin_async(
//     stor_device_t* device,
//     uint64_t block_lba,
//     cache_entry_t** out_entry, 
//     stor_pin_cb_t cb,
//     void* ctx);

// void stor_unpin_async(
//     stor_device_t* device,
//     cache_entry_t* cache_entry,
//     stor_unpin_cb_t cb,
//     void* ctx);

void stor_pin_range_async(
    stor_device_t* device,
    uint64_t block_lba,
    uint64_t count,
    cache_entry_t** out_entries, 
    stor_pin_range_cb_t cb,
    void* ctx);

void stor_unpin_range_async(
    stor_device_t* device,
    cache_entry_t** arr,
    uint64_t count,
    stor_unpin_range_cb_t cb,
    void* ctx);

void stor_pin_range_sync(
    stor_device_t* device,
    uint64_t block_lba,
    uint64_t count,
    cache_entry_t** out_entries);

void stor_unpin_range_sync(
    stor_device_t* device,
    cache_entry_t** arr,
    uint64_t count);

void stor_mark_dirty(cache_entry_t* entry);

void stor_flush_unpinned(stor_device_t* device);

uint64_t calc_blocks_per_bytes(stor_device_t* device, uint64_t offset, uint64_t amount);

void init_block_device(stor_device_t* device);

#endif // __BLOCK_DEVICE_H__