
#include "vfs/core/mount.h"
#include "memory/heap/heap.h"
#include "sys/num_defs.h"
#include "utils/data_structs/flat_hashmap.h"
#include "vfs/core/dir_entry.h"
#include "vfs/core/dir_entry_cache.h"
#include "vfs/core/superblock.h"
#include <string.h>

void vfs_mount_register(
    vfs_mount_map_t* mount_data, 
    dir_entry_t* target, 
    const char* mount_name, 
    dir_entry_t* root)
{
    vfs_mountpoint_t* new_mount = kmalloc(sizeof(vfs_mountpoint_t));

    new_mount->root = root;
    new_mount->mountpoint   = target;
    new_mount->name     = kstrdup(mount_name);

    isize_ptr res = fhashmap_insert(
        &mount_data->mounts, 
        new_mount->mountpoint, 
        sizeof(dir_entry_t*), 
        new_mount, 
        0
    );

    assert(res >= 0);
}

dir_entry_t* vfs_mount_resolve(
    vfs_mount_map_t* mount_data, 
    dir_entry_t* target)
{
    flat_hashmap_result_t result = fhashmap_get_data(
        &mount_data->mounts, 
        target,
        sizeof(dir_entry_t*)
    );

    if (! result.succeed)
    {
        return NULL;
    }

    return result.value;
}

void vfs_mount_set_root(vfs_mount_map_t* mount_tree, superblock_t* root_sb)
{
    assert(root_sb);
    assert(root_sb->driver);
    assert(root_sb->driver->get_root_inode);

    struct inode* root_inode = 
        root_sb->driver->get_root_inode(root_sb);

    assert(root_inode);

    dir_entry_t* root_dentry =
        dir_entry_create(NULL, "", root_inode);

    dir_entry_cache_insert(
        &mount_tree->dcache,
        root_dentry
    );

    mount_tree->root = root_dentry;
}

void vfs_mount_init_tree(vfs_mount_map_t* mount_tree)
{
    mount_tree->root   = NULL;
    mount_tree->mounts = init_fhashmap();
    init_dir_entry_cache(&mount_tree->dcache);
}