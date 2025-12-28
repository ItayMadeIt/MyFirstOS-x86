#ifndef __DIR_ENTRY_CACHE_H__
#define __DIR_ENTRY_CACHE_H__

#include "utils/data_structs/flat_hashmap.h"
#include "vfs/core/dir_entry.h"
#include "vfs/core/superblock.h"

typedef struct dir_entry_cache 
{
    flat_hashmap_t map; // key → dir_entry_t*
} dir_entry_cache_t;

void init_dir_entry_cache(dir_entry_cache_t* cache);
void clean_dir_entry_cache(dir_entry_cache_t* cache);

dir_entry_t* dir_entry_create(
    dir_entry_t* parent, 
    const char* child_name,
    struct inode* inode
);

dir_entry_t* dir_entry_cache_lookup(
    dir_entry_cache_t* cache,
    dir_entry_t* parent,
    const char* child_name
);

void dir_entry_cache_insert(
    dir_entry_cache_t* cache,
    dir_entry_t* entry
);

void dir_entry_cache_drop(
    dir_entry_cache_t* cache,
    dir_entry_t* entry
);

void dir_entry_cache_purge_sb(
    dir_entry_cache_t* cache,
    superblock_t* sb
);

#endif // __DIR_ENTRY_CACHE_H__