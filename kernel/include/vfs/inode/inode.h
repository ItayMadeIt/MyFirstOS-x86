#ifndef __VFS_NODE_H__
#define __VFS_NODE_H__

#include "core/num_defs.h"
#include "core/atomic_defs.h"
#include "vfs/core/defs.h"
#include "vfs/inode/ops.h"
#include <stdatomic.h>

typedef struct inode 
{
    vfs_ops_t* ops;

    void*     fs_id;
    usize_ptr fs_id_length;

    usize size;

    struct superblock* sb;
    
    atom_usize refcount;
    flags_t flags;

    void* fs_internal;
} inode_t;

#endif // __VFS_NODE_H__