#ifndef __FS_DEFS_H__
#define __FS_DEFS_H__

#include "core/num_defs.h"
#include "services/block/device.h"
#include "utils/data_structs/flat_hashmap.h"
#include "vfs/core/dir_entry.h"
#include "vfs/inode/fs_driver.h"
#include "vfs/inode/inode.h"

struct superblock;

typedef struct superblock_ops 
{
    struct inode* (*get_root_inode)(struct superblock* superblock);
    
} superblock_ops_t;

typedef struct superblock
{
    fs_driver_t* driver;
    block_device_t* device;
    void* fs_data;

    flat_hashmap_t inode_cache;

} superblock_t;

#endif // __FS_DEFS_H__