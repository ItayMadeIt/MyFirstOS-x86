#ifndef __VFS_NODE_CACHE_H__
#define __VFS_NODE_CACHE_H__

#include "vfs/core/node.h"

void vfs_cache_insert(vfs_node_t* node);
vfs_node_t* vfs_cache_fetch(vfs_mount_t* mount, void* internal);
void vfs_cache_drop(vfs_node_t* node);

void vfs_cache_purge_mount(vfs_mount_t* mount);

void vfs_node_cache_init();

#endif // __VFS_NODE_CACHE_H__