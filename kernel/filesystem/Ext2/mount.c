
#include "core/num_defs.h"
#include "filesystem/drivers/Ext2/disk.h"
#include "filesystem/drivers/Ext2/driver.h"
#include "filesystem/drivers/Ext2/fs_instance.h"
#include "filesystem/drivers/Ext2/inode.h"
#include "filesystem/drivers/Ext2/internal.h"
#include "kernel/core/cpu.h"
#include "memory/heap/heap.h"
#include "services/block/device.h"
#include "services/block/request.h"
#include "vfs/core/superblock.h"
#include "vfs/inode/inode.h"
#include "vfs/inode/inode_cache.h"
#include "core/defs.h"
#include <string.h>

static  u64 ext2_hash_inode(const u8* inode_num_ptr, u64 length)
{
    assert(length == sizeof(u32));

    u32* data = (u32*)inode_num_ptr;

    return *data;
}

static void ext2_destroy_cb(void* data, void* key_data, usize_ptr key_length)
{
    assert(key_data && key_length);

    inode_t* inode = data;
    ext2_inode_t* ext2_inode = inode->fs_internal;
    kfree(ext2_inode);
    kfree(inode);
}

static void fetch_bgdt(superblock_t* sb)
{
    ext2_fs_instance_t* instance = sb->fs_data;
    if (instance->block_size == 1024)
        instance->bgdt_offset = 2 * instance->block_size;
    else 
        instance->bgdt_offset = 1 * instance->block_size;

    instance->bgd_descs_count = div_up(instance->disk_sb.inodes_count, instance->disk_sb.inodes_per_group);

    u32 descs_size = instance->bgd_descs_count * sizeof(ext2_disk_group_desc_t);
    instance->bgd_descs = kmalloc(descs_size);

    fs_read_bytes(sb, instance->bgd_descs, descs_size, instance->bgdt_offset);
}

static void fetch_root_inode(superblock_t* sb)
{
    ext2_fetch_inode(sb, EXT2_ROOT_INODE);
}

void ext2_mount(superblock_t* sb, block_device_t* block_dev)
{
    sb->device = block_dev;
    sb->driver = &ext2_driver;
    init_inode_cache(
        sb,
        ext2_hash_inode, 
        ext2_destroy_cb
    );

    ext2_fs_instance_t* ext2_instance = kmalloc(sizeof(ext2_fs_instance_t));
    assert(ext2_instance);

    sb->fs_data = ext2_instance;

    fs_read_bytes(
        sb, 
        &ext2_instance->disk_sb, 
        sizeof(ext2_instance->disk_sb), 
        EXT2_SB_OFFSET
    );

    ext2_disk_superblock_t* ext2_sb = &ext2_instance->disk_sb;

    ext2_instance->block_size = EXT2_BLOCK_SIZE(ext2_sb->log_block_size);

    fetch_bgdt(sb);

    fetch_root_inode(sb);
}