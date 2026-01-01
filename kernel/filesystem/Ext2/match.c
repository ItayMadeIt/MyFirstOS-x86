#include "filesystem/drivers/Ext2/disk.h"
#include "filesystem/drivers/Ext2/driver.h"
#include "filesystem/drivers/Ext2/features.h"

#include "core/defs.h"
#include "filesystem/drivers/Ext2/internal.h"
#include "kernel/core/cpu.h"
#include "services/block/device.h"
#include "services/block/request.h"
#include "vfs/core/superblock.h"

bool ext2_match(block_device_t* block_dev)
{
    superblock_t temp_sb = {
        .device = block_dev,
        .driver = &ext2_driver,
        .fs_data = NULL
    };

    ext2_disk_superblock_t ext2_sb;

    fs_read_bytes(
        &temp_sb, 
        &ext2_sb, sizeof(ext2_disk_superblock_t), 
        EXT2_SB_OFFSET
    );

    u32 groups1 = div_up(ext2_sb.inodes_count, ext2_sb.inodes_per_group);
    u32 groups2 = div_up(ext2_sb.blocks_count, ext2_sb.blocks_per_group);

    return ext2_sb.magic == EXT2_MAGIC && groups1 == groups2;
}