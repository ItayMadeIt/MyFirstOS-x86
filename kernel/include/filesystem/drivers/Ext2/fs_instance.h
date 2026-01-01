#ifndef EXT2_FS_INSTANCE_H
#define EXT2_FS_INSTANCE_H

#include "core/num_defs.h"
#include "filesystem/drivers/Ext2/disk.h"

typedef struct ext2_fs_instance
{
    ext2_disk_superblock_t disk_sb;
    
    u32 block_size;
    u32 bgdt_offset;
    u32 bgd_descs_count;
    ext2_disk_group_desc_t* bgd_descs;

} ext2_fs_instance_t;

#endif // EXT2_FS_INSTANCE_H
