#ifndef __BLOCK_DEVICE_H__
#define __BLOCK_DEVICE_H__

#include "utils/data_structs/flat_hashmap.h"
#include "utils/data_structs/rb_tree.h"
#include <stdint.h>
#include "core/num_defs.h"

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

typedef struct pin_range pin_range_t;

typedef struct cache_entry 
{
    // data
    usize lba;
    void* buffer;
    u32 cur_ref_count;
    u8  state;
    bool dirty;

    // data for IO op
    struct {
        pin_range_t* own_range;      // e.g., pin_range_t*
        usize new_lba;     // where to recycle after writeback
        usize new_index;   // index in out array (or in-range offset)
    } io;
    
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

    void**   blocks_arr;      // array of buckets, each one virt addr
    usize_ptr blocks_length;  // number of allocated buckets
    usize_ptr block_size;     // max(page_size, sector_size)
    usize_ptr pages_per_block; // block_size / PAGE_SIZE

    usize_ptr max_bytes;        // max amount of bytes for the entire buckets
    usize_ptr used_bytes;       // track current usage

} block_device_data_t;

typedef void (*stor_pin_cb_t)(void* ctx, int status, cache_entry_t* entry);
typedef void (*stor_unpin_cb_t)(void* ctx, int status);

typedef void (*stor_pin_range_cb_t)(void* ctx, int status, cache_entry_t** entries, u64 count);
typedef void (*stor_unpin_range_cb_t)(void* ctx, int status);


// void stor_pin_async(
//     stor_device_t* device,
//     u64 block_lba,
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
    usize block_lba,
    usize count,
    cache_entry_t** out_entries, 
    stor_pin_range_cb_t cb,
    void* ctx);

void stor_unpin_range_async(
    stor_device_t* device,
    cache_entry_t** arr,
    usize count,
    stor_unpin_range_cb_t cb,
    void* ctx);

void stor_pin_range_sync(
    stor_device_t* device,
    usize block_lba,
    usize count,
    cache_entry_t** out_entries);

void stor_unpin_range_sync(
    stor_device_t* device,
    cache_entry_t** arr,
    usize count);

void stor_mark_dirty(cache_entry_t* entry);

void stor_flush_unpinned(stor_device_t* device);

usize calc_blocks_per_bytes(stor_device_t* device, usize offset, usize amount);

void* block_dev_vbuffer(cache_entry_t* entry);
usize_ptr block_dev_bufsize(stor_device_t* device);

void* stor_clone_vrange(stor_device_t* device, cache_entry_t** entries, usize count);

void init_block_device(stor_device_t* device);

#endif // __BLOCK_DEVICE_H__