#ifndef __VFS_PATH_H__
#define __VFS_PATH_H__

#include "vfs/core/dir_entry.h"
#include "vfs/core/dir_entry_cache.h"
#include "vfs/core/mount.h"

dir_entry_t* vfs_lookup_path(
    vfs_mount_map_t* vfs,
    const char* path
);

#endif // __VFS_PATH_H__