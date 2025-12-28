#include "vfs/inode/inode_cache.h"
#include "utils/data_structs/flat_hashmap.h"
#include "vfs/core/superblock.h"

void inode_cache_insert(superblock_t* sb, inode_t* node)
{
    fhashmap_insert(
        &sb->inode_cache, 
        node->fs_id, 
        node->fs_id_length, 
        node, 
        0
    ); 
}

inode_t* inode_cache_fetch(
    superblock_t* sb,
    const void* fs_id,
    usize_ptr fs_id_length)
{
    flat_hashmap_result_t res = fhashmap_get_data(
        &sb->inode_cache, 
        fs_id, 
        fs_id_length
    );

    if (! res.succeed)
    {
        return NULL;
    }

    return res.value;
}

void inode_cache_drop(superblock_t* sb, inode_t* node)
{
    fhashmap_delete(
        &sb->inode_cache, 
        node->fs_id, 
        node->fs_id_length
    ); 
}

static void del_inode_entry(void* data, void* key, usize_ptr key_length, void* ctx)
{
    (void)data;

    fhashmap_delete(
        ctx, 
        key, 
        key_length
    );
}

void inode_cache_purge_sb(superblock_t* sb)
{
    // theoretically dangerous though
    fhashmap_foreach(
        &sb->inode_cache, 
        del_inode_entry, 
        &sb->inode_cache
    );
}

void init_inode_cache(
    superblock_t* sb, 
    fmap_hash_func fs_inode_hash, 
    fmap_destroy_cb_func inode_destroy_cb)
{
    sb->inode_cache = init_fhashmap_destroy_hash(
        fs_inode_hash, 
        inode_destroy_cb
    );
}