#ifndef __FS_DEFS_H__
#define __FS_DEFS_H__

#include "services/storage/partition.h"
#include "vfs/core/driver.h"

typedef struct vfs_instance
{
    vfs_driver_t* driver;
    stor_partition_t* partition;
    vfs_node_t* vfs_root;
    void* fs_data;

} vfs_instance_t;

vfs_instance_t* vfs_create_instance(stor_partition_t* partition);
void vfs_destroy_instance(vfs_instance_t* fs_instance);

#endif // __FS_DEFS_H__