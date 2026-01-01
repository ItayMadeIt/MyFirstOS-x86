#ifndef __FS_EXT2_INTERNAL_H__
#define __FS_EXT2_INTERNAL_H__

#include "core/num_defs.h"
#include "vfs/core/defs.h"
#include "vfs/core/superblock.h"
#include "vfs/inode/inode.h"

#define EXT2_INODE_INDIRECT_BLOCKS 12

void fs_read_bytes(
    superblock_t* sb, 
    void* data, usize_ptr length, 
    usize offset
);

inode_t* ext2_fetch_inode(
    superblock_t* sb, 
    u32 inode
);

i32 ext2_read_inode(
    inode_t* inode,
    void* vbuffer, 
    usize length,
    off_t file_offset
);

#endif // __FS_EXT2_INTERNAL_H__