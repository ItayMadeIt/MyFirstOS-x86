#include <string.h>
#include <utils/data_structs/flat_hashmap.h>

#include <memory/heap/heap.h>
#include <utils/hash/murmur2_hash.h>

#define INIT_CAPACITY 64
#define MIN_CAPACITY   4

typedef enum fhashmap_entry_state 
{
    STATE_EMPTY  = 0, // MUST BE 0 FOR MEMSET
    STATE_USED   = 1,
    STATE_DELETE = 2,
} fhashmap_entry_state_t; 

typedef enum fhashmap_entry_flag 
{
    FLAG_ALLOCATED = 1 << 0,
} fhashmap_entry_flag_t; 


// Finds the first free slot and returns it
// Assumes empty_count > 0
static flat_hashmap_entry_t* find_free_slot(flat_hashmap_entry_t* entries, uint64_t capacity, uint64_t hash)
{
    uint64_t pos = hash & (capacity-1);

    while (entries[pos].state == STATE_USED)
    {
        pos = (pos+1) & (capacity-1);
    }
    
    return &entries[pos];
}

// Updates the hashmap with the new capacity (can be the same if just a basic rehash)
static intptr_t rehash(flat_hashmap_t* hashmap, uint64_t new_capacity)
{
    uint64_t old_capacity = hashmap->capacity;
    assert(new_capacity >= old_capacity);
    
    // make new entries
    flat_hashmap_entry_t* new_entries = kalloc(sizeof(flat_hashmap_entry_t) * new_capacity);
    assert(new_entries);

    memset(new_entries, 0, sizeof(flat_hashmap_entry_t) * new_capacity);

    flat_hashmap_entry_t* old_entries = hashmap->entries;
    
    uint64_t cur_used_count = 0;
    // delete count is always 0 (point of the rehash)
    // empty count is always new_capacity - used_count (point of the rehash: used_count + empty_count = new_capacity)

    uint64_t used_count = hashmap->used_count;
    
    for (uint64_t pos = 0; pos < old_capacity && cur_used_count < used_count; pos++)
    {
        if (old_entries[pos].state != STATE_USED)
        {
            continue;
        }

        cur_used_count++;

        flat_hashmap_entry_t* old_entry = &old_entries[pos];

        flat_hashmap_entry_t* new_entry =
            find_free_slot(new_entries, new_capacity, old_entries[pos].hash);

        memcpy(new_entry, old_entry, sizeof(flat_hashmap_entry_t));
    }

    hashmap->delete_count = 0;
    hashmap->empty_count  = new_capacity - used_count;
    hashmap->used_count   = used_count;
    hashmap->capacity     = new_capacity;

    kfree(hashmap->entries);
    hashmap->entries = new_entries;

    return 0;
}

// Arbitrary decision of whether a resize is needed
static bool need_resize(flat_hashmap_t *map) 
{
    // load factor (live+deleted entries vs capacity) 75%
    return (map->used_count + map->delete_count) * 100 / map->capacity > 75;
}

// Arbitrary decision of whether a rehash is needed
static bool need_rehash(flat_hashmap_t *map) 
{
    // too many deleted entries relative to used
    return map->delete_count * 100 / (map->used_count + map->delete_count) > 25;
}


static flat_hashmap_entry_t* find_exact_entry_interval(flat_hashmap_t* hashmap, flat_hashmap_entry_t* from, flat_hashmap_entry_t* to, uint64_t hash, const uint8_t* key_data, uint64_t key_length)
{
    uint64_t from_index = from - hashmap->entries;
    uint64_t to_index   = to - hashmap->entries;

    uint64_t cur_index = from_index;

    while (cur_index != to_index)
    {
        if (hashmap->entries[cur_index].state == STATE_EMPTY) 
        {
            return NULL;
        }

        if (hashmap->entries[cur_index].hash == hash && 
            hashmap->entries[cur_index].key_length == key_length &&
            memcmp(hashmap->entries[cur_index].key_data, key_data, key_length) == 0)
        {
            return &hashmap->entries[cur_index];
        }

        cur_index = (cur_index + 1) & (hashmap->capacity - 1);
    }

    return NULL;
}

static flat_hashmap_entry_t* find_exact_entry(flat_hashmap_t* hashmap, uint64_t hash, const uint8_t* key_data, uint64_t key_length)
{
    assert(hashmap->empty_count != 0);

    uint64_t pos = hash & (hashmap->capacity - 1);

    while (1)
    {
        flat_hashmap_entry_t* it = &hashmap->entries[pos];

        if (it->state == STATE_EMPTY) 
        {
            return NULL;
        }

        if (it->state == STATE_USED &&
            it->hash == hash &&
            it->key_length == key_length &&
            memcmp(it->key_data, key_data, key_length) == 0)
        {
            return it;
        }

        pos = (pos + 1) & (hashmap->capacity - 1);
    }

    return NULL;
}

static void clean_entry(flat_hashmap_entry_t* entry, uint8_t new_state)
{
    if (entry->flags & FLAG_ALLOCATED)
    {
        kfree((void*)entry->key_data);
    }

    entry->key_data   = NULL;
    entry->key_length = 0;

    entry->hash = 0;
    entry->flags = 0;
    entry->state = new_state;
}

static intptr_t handle_rehash(flat_hashmap_t* hashmap)
{
    if (need_resize(hashmap))
    {
        intptr_t rehash_result = rehash(hashmap, hashmap->capacity * 2);
        if (rehash_result < 0)
        {
            return rehash_result;
        }
    }
    else if (need_rehash(hashmap))
    {
        intptr_t rehash_result = rehash(hashmap, hashmap->capacity);
        if (rehash_result < 0)
        {
            return rehash_result;
        }
    }

    return 0;
}


flat_hashmap_t init_fhashmap_custom(uint64_t capacity, hash_func hash)
{
    capacity = align_up_pow2(capacity);
    assert(capacity);

    if (capacity < MIN_CAPACITY)
    {
        capacity = MIN_CAPACITY;
    }

    flat_hashmap_entry_t* entries = kalloc(capacity * sizeof(flat_hashmap_entry_t));
    assert(entries);
    memset(entries, 0, capacity * sizeof(flat_hashmap_entry_t));

    flat_hashmap_t map = {
        .capacity = capacity,
        .empty_count = capacity,
        .delete_count = 0,
        .used_count = 0,
        .entries = entries,
        .hash = hash
    };

    return map;
}

flat_hashmap_t init_fhashmap_capacity(uint64_t capacity)
{
    return init_fhashmap_custom(capacity, murmur2_hash64);
}

flat_hashmap_t init_fhashmap(void)
{
    return init_fhashmap_capacity(INIT_CAPACITY);
}

intptr_t fhashmap_insert(flat_hashmap_t* hashmap, const uint8_t *key_data, uint64_t key_length, void *data, uint8_t flags)
{
    uint64_t hash = hashmap->hash(key_data, key_length);

    flat_hashmap_entry_t* entry = find_free_slot(
        hashmap->entries, 
        hashmap->capacity, 
        hash
    );

    flat_hashmap_entry_t* ideal_slot = &hashmap->entries[hash & (hashmap->capacity-1)];

    flat_hashmap_entry_t* same_entry = find_exact_entry_interval(
        hashmap, 
        ideal_slot,
        entry, 
        hash, 
        key_data, 
        key_length
    );

    if (same_entry)
    {
        if ((flags & FHASHMAP_INS_FLAG_OVERWRITE) == false)
            return -1;

        // Clears that entry
        clean_entry(same_entry, STATE_EMPTY);
        hashmap->used_count--; 
        hashmap->empty_count++; 

        entry = same_entry;
    }

    // Now adding a new entry 
    hashmap->used_count ++;
    if (entry->state == STATE_EMPTY)
        hashmap->empty_count--;
    else if (entry->state == STATE_DELETE)
        hashmap->delete_count--;

    if (flags & FHASHMAP_INS_FLAG_KEY_ALLOCATED)
    {
        void* new_key = kalloc(key_length);
        memcpy(new_key, key_data, key_length);

        entry->flags |= FLAG_ALLOCATED;
        entry->key_data = new_key;
    }
    else
    {
        entry->flags &= ~FLAG_ALLOCATED;
        entry->key_data = key_data;
    }

    entry->key_length = key_length;
    entry->hash = hash;

    entry->data = data;

    entry->state = STATE_USED;

    return handle_rehash(hashmap);
}

void* fhashmap_get_data(flat_hashmap_t* hashmap, const uint8_t *key_data, uint64_t key_length)
{
    uint64_t hash = hashmap->hash(key_data, key_length);

    flat_hashmap_entry_t* same_entry = find_exact_entry(hashmap, hash, key_data, key_length);

    if (same_entry)
    {
        return same_entry->data;
    }
    else
    {
        return NULL;
    }
}

intptr_t fhashmap_delete(flat_hashmap_t* hashmap, const uint8_t *key_data, uint64_t key_length)
{
    uint64_t hash = hashmap->hash(key_data, key_length);

    flat_hashmap_entry_t* same_entry = find_exact_entry(hashmap, hash, key_data, key_length);

    if (!same_entry)
    {
        return -1;
    }

    clean_entry(same_entry, STATE_DELETE);

    hashmap->delete_count++;
    hashmap->used_count--;

    return handle_rehash(hashmap);
}

void fhashmap_clear(flat_hashmap_t* hashmap)
{
    for (uint32_t i = 0; i < hashmap->capacity; i++)
    {
        if (hashmap->entries[i].flags & FLAG_ALLOCATED)
        {
            kfree((void*)hashmap->entries[i].key_data);
        }
    }
    
    memset(hashmap->entries, 0, hashmap->capacity * sizeof(flat_hashmap_entry_t));

    hashmap->delete_count = 0;
    hashmap->used_count   = 0;
    hashmap->empty_count  = hashmap->capacity;
}

void fhashmap_free(flat_hashmap_t* hashmap)
{
    for (uint32_t i = 0; i < hashmap->capacity; i++)
    {
        if (hashmap->entries[i].flags & FLAG_ALLOCATED)
        {
            kfree((void*)hashmap->entries[i].key_data);
        }
    }

    kfree(hashmap->entries);
}