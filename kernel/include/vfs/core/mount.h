#ifndef __VFS_MOUNT_H__
#define __VFS_MOUNT_H__

#include "vfs/core/driver.h"
#include "node.h"

typedef struct vfs_mount 
{
    const char* name;
    
    vfs_node_t* target; 

    vfs_instance_t* fs_instance; // if real fs, otherwise NULL
    vfs_node_t* root;
} vfs_mount_t;

void vfs_mount_register(vfs_node_t* target, const char* mount_name, vfs_node_t* root);
vfs_node_t* vfs_mount_resolve(vfs_node_t* target); 

#endif // __VFS_MOUNT_H__