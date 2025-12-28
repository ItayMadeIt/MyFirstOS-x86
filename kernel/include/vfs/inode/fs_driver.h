#ifndef __FS_DRIVER_H__
#define __FS_DRIVER_H__

#include "services/block/device.h"
#include "vfs/inode/inode.h"

struct superblock;

typedef struct fs_driver 
{
    const char* name;

    bool (*match)(const block_device_t* device);
    void (*mount)(struct superblock* fs_instance, block_device_t* block_dev);
    struct inode* (*read_inode)(struct superblock* sb, void* internal);
    struct inode* (*get_root_inode)(struct superblock* sb);

} fs_driver_t;

#endif // __FS_DRIVER_H__