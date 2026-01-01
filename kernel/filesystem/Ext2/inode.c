#include "filesystem/drivers/Ext2/inode.h"

#include "core/num_defs.h"
#include "filesystem/drivers/Ext2/disk.h"
#include "filesystem/drivers/Ext2/features.h"
#include "filesystem/drivers/Ext2/fs_instance.h"
#include "filesystem/drivers/Ext2/internal.h"
#include "kernel/core/cpu.h"
#include "memory/heap/heap.h"
#include "vfs/core/superblock.h"
#include "vfs/inode/inode.h"
#include "vfs/inode/ops.h"
#include <string.h>

void ext2_disk_done_cb(struct block_request* request, i64 result)
{
    assert(result >= 0);
    
    bool* done = request->ctx;
    *done = true;
}

static u32 ext2_get_block_group(superblock_t* sb, u32 inode_num)
{
    ext2_fs_instance_t* fs_instance = (ext2_fs_instance_t*) sb->fs_data;
    assert(inode_num <= fs_instance->disk_sb.inodes_count);

    return (inode_num - 1) / fs_instance->disk_sb.inodes_per_group;
}

static vfs_ops_t ext2_ops = {
    .cleanup  = NULL,
    
    .lookup   = ext2_vfs_lookup,

    .read     = ext2_vfs_read,
    .write    = NULL,
    
    .mkdir    = NULL,
    .rmdir    = NULL,
    
    .link     = NULL,
    .symlink  = NULL,
    .readlink = NULL,
    
    .create   = NULL,
    .open     = NULL,
    .close    = NULL,

    .stat     = NULL,

    .rename   = NULL,
};

inode_t* make_init_inode(superblock_t *sb, u32 inode_num)
{
    ext2_fs_instance_t* fs_instance = (ext2_fs_instance_t*) sb->fs_data;
    u32 bgd_index = ext2_get_block_group(sb, inode_num);

    u32 inode_table_block = fs_instance->bgd_descs[bgd_index].inode_table;
    
    u32 index_in_group = (inode_num - 1) % fs_instance->disk_sb.inodes_per_group;

    usize inode_offset = 
        inode_table_block * fs_instance->block_size
        + fs_instance->disk_sb.inode_size * index_in_group;

    ext2_inode_t* ext2_inode = kmalloc(sizeof(ext2_inode_t));
    assert(ext2_inode);
    ext2_inode->inode_num = inode_num;

    usize_ptr inode_size = fs_instance->disk_sb.inode_size;

    inode_t* inode = kmalloc(sizeof(inode_t));

    inode->fs_id = &ext2_inode->inode_num;
    inode->fs_id_length = sizeof(ext2_inode->inode_num);
    inode->fs_internal = ext2_inode;
    inode->refcount = 1;
    inode->sb = sb;
    inode->ops = &ext2_ops;

    u8 inode_as_bytes[inode_size];
    fs_read_bytes(sb, inode_as_bytes, inode_size, inode_offset);

    memcpy(&ext2_inode->disk, inode_as_bytes, sizeof(ext2_inode->disk));
    inode->size = ext2_inode->disk.size;

    return inode;
}