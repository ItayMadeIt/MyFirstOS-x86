#ifndef __DRIVERS_H__
#define __DRIVERS_H__

#include "vfs/inode/fs_driver.h"

typedef u16 driver_count_t;

extern const driver_count_t fs_drivers_count;
extern fs_driver_t* fs_drivers[];

#endif // __DRIVERS_H__