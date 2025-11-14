#ifndef __FS_DRIVER_H__
#define __FS_DRIVER_H__

#include "services/storage/partition.h"
#include "vfs/core/node.h"

typedef struct vfs_instance vfs_instance_t ;

typedef struct vfs_driver 
{
    const char* name;
    vfs_ops_t* ops;
    
    bool (*match_driver)(stor_partition_t* partition);
    void (*init_fs_instance)(vfs_instance_t* fs_instance);

} vfs_driver_t;

#endif // __FS_DRIVER_H__