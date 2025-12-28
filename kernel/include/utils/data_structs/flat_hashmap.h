#ifndef __UTILS_FLAT_HASHMAP_H__
#define __UTILS_FLAT_HASHMAP_H__

#include "core/num_defs.h"
#include <stdbool.h>

typedef struct flat_hashmap_result
{
    void* value;
    bool succeed;
} flat_hashmap_result_t;

typedef struct flat_hashmap_entry
{
    u64 hash;

    void* key_data;
    u64 key_length;

    void* data;

    u8 state;

    u8 flags; // is key allocted here
} flat_hashmap_entry_t;

typedef u64 (*fmap_hash_func)(const u8* data, u64 length);
typedef void (*fmap_destroy_cb_func)(void* data, void* key_data, usize_ptr key_length);

typedef struct flat_hashmap
{
    flat_hashmap_entry_t* entries;

    u64 empty_count;
    u64 used_count;
    u64 delete_count;

    u64 capacity;

    fmap_hash_func hash;
    fmap_destroy_cb_func destroy_cb;

} flat_hashmap_t;

flat_hashmap_t init_fhashmap             (void);
flat_hashmap_t init_fhashmap_capacity    (u64 capacity);
flat_hashmap_t init_fhashmap_custom      (u64 capacity, fmap_hash_func hash, fmap_destroy_cb_func destroy_cb);
flat_hashmap_t init_fhashmap_destroy_hash(fmap_hash_func hash, fmap_destroy_cb_func destroy_cb);
flat_hashmap_t init_fhashmap_destroy     (fmap_destroy_cb_func destroy_cb);
flat_hashmap_t init_fhashmap_hash        (fmap_hash_func hash);


#define FHASHMAP_INS_FLAG_KEY_COPY   (1<<0)  // hashmap allocates + copies key
#define FHASHMAP_INS_FLAG_OVERWRITE  (1<<1)  // overwrite value if key already exists

isize_ptr fhashmap_insert(flat_hashmap_t* hashmap, void* key_data, u64 key_length, void* data, u8 flags);

typedef void (*fhashmap_foreach_entry_cb)(void* data, void* key, usize_ptr key_length, void* ctx);

void fhashmap_foreach(flat_hashmap_t* hashmap, fhashmap_foreach_entry_cb, void* ctx);

flat_hashmap_result_t fhashmap_get_data (flat_hashmap_t* hashmap, const void *key_data, u64 key_length);

flat_hashmap_result_t fhashmap_delete(flat_hashmap_t* hashmap, const void *key_data, u64 key_length);

void fhashmap_clear(flat_hashmap_t* hashmap);

void fhashmap_free(flat_hashmap_t* hashmap);

#endif // __UTILS_FLAT_HASHMAP_H__