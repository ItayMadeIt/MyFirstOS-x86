#ifndef __FS_NODE_CACHE_H__
#define __FS_NODE_CACHE_H__

#include "utils/data_structs/flat_hashmap.h"
#include "vfs/inode/inode.h"
#include "vfs/core/superblock.h"

void inode_cache_insert(superblock_t* sb, inode_t* node);

inode_t* inode_cache_fetch(
    superblock_t* sb, 
    const void* fs_id,
    usize_ptr fs_id_length
);

void inode_cache_drop(superblock_t* sb, inode_t* node);

void inode_cache_purge_sb(superblock_t* sb);

void init_inode_cache(
    superblock_t* sb, 
    fmap_hash_func fs_inode_hash, 
    fmap_destroy_cb_func inode_destroy_cb
);

#endif // __FS_NODE_CACHE_H__