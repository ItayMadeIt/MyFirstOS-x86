#ifndef __FLAT_HASHMAP_H__
#define __FLAT_HASHMAP_H__

#include "core/num_defs.h"
#include <stdbool.h>
#include "core/num_defs.h"

typedef struct flat_hashmap_result
{
    void* value;
    bool succeed;
} flat_hashmap_result_t;

typedef struct flat_hashmap_entry
{
    u64 hash;

    const void* key_data;
    u64 key_length;

    void* data;

    u8 state;

    u8 flags; // is key allocted here
} flat_hashmap_entry_t;

typedef u64 (*hash_func)(const u8* data, u64 length);

typedef struct flat_hashmap
{
    flat_hashmap_entry_t* entries;

    u64 empty_count;
    u64 used_count;
    u64 delete_count;

    u64 capacity;

    hash_func hash;

} flat_hashmap_t;

flat_hashmap_t init_fhashmap         (void);
flat_hashmap_t init_fhashmap_capacity(u64 capacity);
flat_hashmap_t init_fhashmap_custom  (u64 capacity, hash_func hash);


#define FHASHMAP_INS_FLAG_KEY_ALLOCATED  (1<<0)  // hashmap allocates + copies key
#define FHASHMAP_INS_FLAG_OVERWRITE      (1<<1)  // overwrite value if key already exists

ssize_ptr fhashmap_insert(flat_hashmap_t* hashmap, const void* key_data, u64 key_length, void* data, u8 flags);

flat_hashmap_result_t fhashmap_get_data (flat_hashmap_t* hashmap, const void *key_data, u64 key_length);

flat_hashmap_result_t fhashmap_delete(flat_hashmap_t* hashmap, const void *key_data, u64 key_length);

void fhashmap_clear(flat_hashmap_t* hashmap);

void fhashmap_free(flat_hashmap_t* hashmap);

#endif // __FLAT_HASHMAP_H__