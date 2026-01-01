#include "vfs/core/dir_entry_cache.h"

#include "core/num_defs.h"
#include "memory/heap/heap.h"
#include "sys/num_defs.h"
#include "utils/data_structs/flat_hashmap.h"
#include "utils/hash/murmur2_hash.h"
#include "vfs/core/dir_entry.h"
#include "vfs/core/superblock.h"
#include <string.h>

dir_entry_t* dir_entry_create(
    dir_entry_t* parent, 
    const char* child_name,
    struct inode* inode)
{
    usize_ptr child_name_len = strlen(child_name);
    assert(child_name_len <= MAX_NODE_NAME_LENGTH);

    dir_entry_t* entry = kmalloc(sizeof(dir_entry_t) + child_name_len + 1);   
    assert(entry);

    memcpy(entry->name, child_name, child_name_len + 1);
    entry->inode  = inode;
    entry->parent = parent;

    return entry;
}

typedef struct dentry_key {
    dir_entry_t* parent;
    u64          name_len;
    char         name[];
} dentry_key_t;

static u64 dentry_hash(const u8* key_data, u64 len)
{
    (void)len;

    const dentry_key_t* key = (const dentry_key_t*) key_data;

    u64 h = murmur2_hash64((u8*)&key->parent, sizeof(dir_entry_t*));
    h ^= murmur2_hash64((u8*)key->name, key->name_len);

    return h;
}

dir_entry_t* dir_entry_cache_lookup(
    dir_entry_cache_t* cache,
    dir_entry_t* parent,
    const char* child_name)
{
    usize_ptr name_len = strlen(child_name);
    assert(name_len <= MAX_NODE_NAME_LENGTH);

    const usize_ptr key_length = sizeof(dentry_key_t) + name_len + 1;
    u8 key_as_arr[key_length];

    dentry_key_t* key = (dentry_key_t*) key_as_arr;
    *key = (dentry_key_t){
        .name_len = name_len,
        .parent   = parent
    };
    memcpy(key->name, child_name, name_len + 1);

    flat_hashmap_result_t res = fhashmap_get_data(
        &cache->map, 
        key, 
        key_length
    );

    if (res.succeed)
    {
        return res.value;
    }
    else
    {
        return NULL;
    }
}

void dir_entry_cache_insert(
    dir_entry_cache_t* cache,
    dir_entry_t* entry)
{
    usize_ptr name_len = strlen(entry->name);
    assert(name_len <= MAX_NODE_NAME_LENGTH);

    const usize_ptr key_length = sizeof(dentry_key_t) + name_len + 1;
    u8 key_as_arr[key_length];

    dentry_key_t* key = (dentry_key_t*) key_as_arr;
    *key = (dentry_key_t){
        .name_len = name_len,
        .parent   = entry->parent
    };
    memcpy(key->name, entry->name, name_len + 1);

    fhashmap_insert(
        &cache->map, 
        key, key_length, 
        entry, 
        FHASHMAP_INS_FLAG_KEY_COPY
    ); 
}

void dir_entry_cache_drop(
    dir_entry_cache_t* cache,
    dir_entry_t* entry)
{
    usize_ptr name_len = strlen(entry->name);
    assert(name_len <= MAX_NODE_NAME_LENGTH);

    const usize_ptr key_length = sizeof(dentry_key_t) + name_len + 1;
    u8 key_as_arr[key_length];

    dentry_key_t* key = (dentry_key_t*) key_as_arr;
    *key = (dentry_key_t){
        .name_len = name_len,
        .parent   = entry->parent
    };
    memcpy(key->name, entry->name, name_len + 1);

    fhashmap_delete(
        &cache->map,
        key, key_length
    );
}

struct purge_entry_dir_entry_data 
{
    dir_entry_cache_t* cache;
    superblock_t* sb;
};

static void dir_purge_entry(void* dir_entry_data, void* key_data, usize_ptr key_length, void* ctx)
{
    dir_entry_t* data = (dir_entry_t*) dir_entry_data;

    struct purge_entry_dir_entry_data* purge_data = ctx;

    if (data->inode->sb == purge_data->sb)
    {
        fhashmap_delete(&purge_data->cache->map, key_data, key_length);
    }
}

void dir_entry_cache_purge_sb(
    dir_entry_cache_t* cache,
    superblock_t* sb)
{
    struct purge_entry_dir_entry_data purge_data = {
        .cache = cache,
        .sb = sb,
    };
        
    fhashmap_foreach(&cache->map, dir_purge_entry, &purge_data);
}

static void on_destroy(void* data, void* key_data, usize_ptr key_length)
{
    (void)key_data; (void)key_length;

    kfree(data);
}

void init_dir_entry_cache(dir_entry_cache_t* cache)
{
    cache->map = init_fhashmap_destroy_hash(dentry_hash, on_destroy);
}

void clean_dir_entry_cache(dir_entry_cache_t* cache)
{
    fhashmap_clear(&cache->map);
}
