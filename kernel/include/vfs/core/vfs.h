#ifndef __VFS_H__
#define __VFS_H__

#include "vfs/core/driver.h"

extern vfs_driver_t* vfs_drivers[];

void vfs_set_root_instance(vfs_instance_t* main_instance);

#endif // __VFS_H__