#ifndef __VFS_NODE_H__
#define __VFS_NODE_H__

#include "core/num_defs.h"
#include "core/atomic_defs.h"
#include "vfs/core/defs.h"
#include <stdatomic.h>

typedef struct vfs_node vfs_node_t;

typedef struct vfs_stat 
{
    usize size;
    u64 inode;
    mode_t mode;
    id_t uid;
    id_t gid;
    u64 access_time;
    u64 mod_time;
    u64 create_time;
} vfs_stat_t;

typedef struct vfs_ops
{
    i32 (*cleanup)(vfs_node_t* node);


    vfs_node_t* (*lookup)(vfs_node_t* dir, const char* name);
    
    i32 (*read)  (vfs_node_t* node, void* buf, size_t size, off_t offset);
    i32 (*write) (vfs_node_t* node, const void* buf, size_t size, off_t offset);
    
    i32 (*mkdir) (vfs_node_t* dir, const char* name, mode_t mode);
    i32 (*rmdir) (vfs_node_t* dir, const char* name, mode_t mode);
    
    i32 (*link)(vfs_node_t* dir, const char* existing, const char* new_name);
    i32 (*symlink)(vfs_node_t* dir, const char* target, const char* link_name);
    i32 (*readlink)(vfs_node_t* node, char* buf, size_t size);

    i32 (*create)(vfs_node_t* dir, const char* name, mode_t mode);
    i32 (*open)  (vfs_node_t* node, flags_t flags);
    i32 (*close) (vfs_node_t* node);

    vfs_stat_t (*stat) (vfs_node_t* node);

    i32 (*rename)(vfs_node_t* dir, const char* old_name,
                                 const char* new_name);
} vfs_ops_t;

typedef struct vfs_mount vfs_mount_t;

typedef struct vfs_node 
{
    char name[MAX_NODE_NAME_LENGTH];
    struct vfs_node* parent;

    vfs_ops_t* ops;
    void* internal;
    u64 inode;

    vfs_mount_t* mount;
    
    atom_usize refcount;
    flags_t flags;
} vfs_node_t;
#endif // __VFS_NODE_H__