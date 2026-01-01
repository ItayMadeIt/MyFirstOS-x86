#ifndef __FS_EXT2_FEATURES_H__
#define __FS_EXT2_FEATURES_H__

#include "vfs/inode/inode.h"
#include <stdbool.h>

struct block_device;
struct superblock;

bool ext2_match(struct block_device* block_dev);
void ext2_mount(struct superblock* sb, struct block_device* block_dev);

inode_t* ext2_get_root_inode(struct superblock* sb);

i32 ext2_vfs_read(
    struct inode* inode, 
    void* vbuffer, 
    usize size, 
    off_t offset
);

i32 ext2_vfs_lookup(
    struct inode* inode, 
    const char* child_str, 
    struct inode** child_ptr
);

#endif // __FS_EXT2_FEATURES_H__