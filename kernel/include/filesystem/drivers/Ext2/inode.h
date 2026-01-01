#ifndef __FS_EXT2_INODE_H__
#define __FS_EXT2_INODE_H__

#include "filesystem/drivers/Ext2/disk.h"
#include "vfs/core/superblock.h"
#include "vfs/inode/inode.h"

#define EXT2_ROOT_INODE 2

typedef struct ext2_inode
{
    ext2_inode_disk_t disk;

    u32 inode_num;

} ext2_inode_t;

inode_t* make_init_inode(superblock_t* sb, u32 inode_num);

#endif // __FS_EXT2_INODE_H__