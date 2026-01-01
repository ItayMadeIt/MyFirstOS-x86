#include "core/defs.h"
#include "core/num_defs.h"
#include "filesystem/drivers/Ext2/fs_instance.h"
#include "filesystem/drivers/Ext2/inode.h"
#include "filesystem/drivers/Ext2/internal.h"
#include "vfs/core/defs.h"
#include "vfs/core/errors.h"
#include "vfs/core/superblock.h"
#include "vfs/inode/inode.h"
#include "vfs/inode/inode_cache.h"

static inline u32 ext2_read_u32(
    superblock_t* sb,
    usize block_size,
    u32 block,
    u32 index)
{
    u32 value;
    fs_read_bytes(
        sb,
        &value,
        sizeof(u32),
        (usize)block * block_size + index * sizeof(u32)
    );
    return value;
}

inode_t* ext2_fetch_inode(superblock_t* sb, u32 inode)
{
    inode_t* cache_inode = inode_cache_fetch(
        sb, 
        &inode, 
        sizeof(inode)
    );

    if (cache_inode)
        return cache_inode;

    inode_t* new_inode = make_init_inode(sb, inode);
    inode_cache_insert(sb, new_inode);

    return new_inode;
}



static u32 ext2_get_inode_block_index(
    inode_t* inode,
    u32 index)
{
    ext2_fs_instance_t* fs = inode->sb->fs_data;
    ext2_inode_t* ext2_inode = inode->fs_internal;

    u32 block_size = fs->block_size;
    u32 ptrs = block_size / sizeof(u32);

    // direct 
    if (index < EXT2_INODE_INDIRECT_BLOCKS)
        return ext2_inode->disk.block[index];

    index -= EXT2_INODE_INDIRECT_BLOCKS;

    // single indirect 
    if (index < ptrs)
    {
        u32 block = ext2_inode->disk.block[EXT2_INODE_INDIRECT_BLOCKS];
        return ext2_read_u32(inode->sb, block_size, block, index);
    }

    index -= ptrs;

    // double indirect 
    if (index < ptrs * ptrs)
    {
        u32 double_block = ext2_inode->disk.block[EXT2_INODE_INDIRECT_BLOCKS + 1];

        u32 index1 = index / ptrs;
        u32 index2 = index % ptrs;

        u32 block = ext2_read_u32(inode->sb, block_size, double_block, index1);
        return ext2_read_u32(inode->sb, block_size, block, index2);
    }

    index -= ptrs * ptrs;

    // triple indirect 
    u32 triple_block = ext2_inode->disk.block[EXT2_INODE_INDIRECT_BLOCKS + 2];

    u32 index1 = index / (ptrs * ptrs);

    u32 remainder = index % (ptrs * ptrs);
    u32 index2 = remainder / ptrs;
    u32 index3 = remainder % ptrs;

    u32 double_block = ext2_read_u32(inode->sb, block_size, triple_block, index1);
    u32 block  = ext2_read_u32(inode->sb, block_size, double_block, index2);
    return ext2_read_u32(inode->sb, block_size, block, index3);
}

i32 ext2_read_inode(
    inode_t* inode,
    void* vbuffer, 
    usize length,
    off_t file_offset)
{
    ext2_inode_t* ext2_inode = inode->fs_internal;
    if (length + file_offset > ext2_inode->disk.size)
    {
        return -VFS_ERR_INVAL;
    }

    superblock_t* sb = inode->sb;
    ext2_fs_instance_t* fs = inode->sb->fs_data;

    usize block_size = fs->block_size;

    usize remaining_length = length;

    void* cur_buffer = vbuffer;

    while (remaining_length > 0)
    {
        u32   file_block_index = file_offset / block_size;
        usize file_block_off   = file_offset % block_size;

        usize chunk_length = min(block_size - file_block_off, remaining_length);

        u32 block_index = ext2_get_inode_block_index(inode, file_block_index);
        if (block_index == 0)
            break; // EOF

        fs_read_bytes(
            sb,
            cur_buffer,
            chunk_length,
            (usize)block_index * block_size + file_block_off
        );

        vbuffer          += chunk_length;
        file_offset      += chunk_length;
        remaining_length -= length;
    }

    return length;
}