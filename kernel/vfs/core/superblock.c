#include "vfs/core/superblock.h"
#include "memory/heap/heap.h"
#include "vfs/inode/inode_cache.h"
#include <string.h>

superblock_t* create_empty_superblock(block_device_t* device)
{
    superblock_t* sb = kmalloc(sizeof(superblock_t));
    assert(sb);

    sb->device  = device;
    sb->driver  = NULL;
    sb->fs_data = NULL;

    // no real init
    memset(&sb->inode_cache, 0, sizeof(sb->inode_cache));

    return sb;
}