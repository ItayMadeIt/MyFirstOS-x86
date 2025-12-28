#include "filesystem/drivers/Ext2/driver.h"
#include "core/defs.h"

fs_driver_t ext2_driver = {
    .name           = "Ext2",
    .match          = NULL,
    .mount          = NULL,
    .read_inode     = NULL,
    .get_root_inode = NULL
};