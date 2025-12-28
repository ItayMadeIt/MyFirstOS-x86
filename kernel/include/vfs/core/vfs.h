#ifndef __VFS_H__
#define __VFS_H__

#include "services/block/device.h"
#include "vfs/core/mount.h"
#include "vfs/core/superblock.h"

extern fs_driver_t* vfs_drivers[];

vfs_mount_map_t* vfs_get_mount_data();
void init_vfs(block_device_t* block_dev);

#endif // __VFS_H__