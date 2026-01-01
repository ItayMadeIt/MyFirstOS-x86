#ifndef __VFS_OPS_H__
#define __VFS_OPS_H__

#include "core/num_defs.h"
#include "vfs/core/defs.h"

struct inode;

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
    i32 (*cleanup)(struct inode* node);


    i32 (*lookup)(struct inode* dir, const char* name, struct inode** result);
    
    i32 (*read)  (struct inode* node, void* buf, usize size, off_t offset);
    i32 (*write) (struct inode* node, const void* buf, usize size, off_t offset);
    
    i32 (*mkdir) (struct inode* dir, const char* name, mode_t mode);
    i32 (*rmdir) (struct inode* dir, const char* name, mode_t mode);
    
    i32 (*link)(struct inode* dir, const char* existing, const char* new_name);
    i32 (*symlink)(struct inode* dir, const char* target, const char* link_name);
    i32 (*readlink)(struct inode* node, char* buf, usize size);

    i32 (*create)(struct inode* dir, const char* name, mode_t mode);
    i32 (*open)  (struct inode* node, flags_t flags);
    i32 (*close) (struct inode* node);

    vfs_stat_t (*stat) (struct inode* node);

    i32 (*rename)(struct inode* dir, const char* old_name,
                                 const char* new_name);
} vfs_ops_t;

#endif // __VFS_OPS_H__