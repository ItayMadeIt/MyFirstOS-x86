#include "filesystem/drivers/Ext2/driver.h"
#include "core/defs.h"
#include "filesystem/drivers/Ext2/features.h"

fs_driver_t ext2_driver = {
    .name           = "Ext2",
    .match          = ext2_match,
    .mount          = ext2_mount,
    .read_inode     = NULL,
    .get_root_inode = ext2_get_root_inode
};