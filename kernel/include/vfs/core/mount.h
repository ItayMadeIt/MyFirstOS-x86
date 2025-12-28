#ifndef __VFS_MOUNT_H__
#define __VFS_MOUNT_H__

#include "utils/data_structs/flat_hashmap.h"
#include "vfs/core/dir_entry.h"
#include "vfs/core/dir_entry_cache.h"
#include "vfs/core/superblock.h"
#include "vfs/inode/fs_driver.h"
#include "vfs/inode/inode.h"

typedef struct vfs_mountpoint 
{
    const char* name;
    
    struct vfs_mountpoint* parent;
    dir_entry_t* mountpoint;

    dir_entry_t* root;
    superblock_t* sb;
} vfs_mountpoint_t;

typedef struct vfs_mount_tree
{
    dir_entry_t* root; 
    flat_hashmap_t mounts;
    dir_entry_cache_t dcache;
} vfs_mount_map_t;

void vfs_mount_register(
    vfs_mount_map_t* data, 
    dir_entry_t* target, 
    const char* mount_name, 
    dir_entry_t* root
);

dir_entry_t* vfs_mount_resolve(
    vfs_mount_map_t* data, 
    dir_entry_t* target
); 

void vfs_mount_init_tree(vfs_mount_map_t* mount_tree);
void vfs_mount_set_root(vfs_mount_map_t* mount_tree, superblock_t* root_sb);

#endif // __VFS_MOUNT_H__