#include "vfs/core/drivers.h"
#include "filesystem/drivers/Ext2/driver.h"

fs_driver_t* fs_drivers[] = {
    &ext2_driver
};
const u16 fs_drivers_count = sizeof(fs_drivers) / sizeof(fs_driver_t*);
