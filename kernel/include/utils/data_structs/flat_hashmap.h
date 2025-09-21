#ifndef __FLAT_HASHMAP_H__
#define __FLAT_HASHMAP_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct flat_hashmap_result
{
    void* value;
    bool succeed;
} flat_hashmap_result_t;

typedef struct flat_hashmap_entry
{
    uint64_t hash;

    const void* key_data;
    uint64_t key_length;

    void* data;

    uint8_t state;

    uint8_t flags; // is key allocted here
} flat_hashmap_entry_t;

typedef uint64_t (*hash_func)(const uint8_t* data, uint64_t length);

typedef struct flat_hashmap
{
    flat_hashmap_entry_t* entries;

    uint64_t empty_count;
    uint64_t used_count;
    uint64_t delete_count;

    uint64_t capacity;

    hash_func hash;

} flat_hashmap_t;

flat_hashmap_t init_fhashmap         (void);
flat_hashmap_t init_fhashmap_capacity(uint64_t capacity);
flat_hashmap_t init_fhashmap_custom  (uint64_t capacity, hash_func hash);


#define FHASHMAP_INS_FLAG_KEY_ALLOCATED  (1<<0)  // hashmap allocates + copies key
#define FHASHMAP_INS_FLAG_OVERWRITE      (1<<1)  // overwrite value if key already exists

intptr_t fhashmap_insert(flat_hashmap_t* hashmap, const void* key_data, uint64_t key_length, void* data, uint8_t flags);

flat_hashmap_result_t fhashmap_get_data (flat_hashmap_t* hashmap, const void *key_data, uint64_t key_length);

flat_hashmap_result_t fhashmap_delete(flat_hashmap_t* hashmap, const void *key_data, uint64_t key_length);

void fhashmap_clear(flat_hashmap_t* hashmap);

void fhashmap_free(flat_hashmap_t* hashmap);

#endif // __FLAT_HASHMAP_H__