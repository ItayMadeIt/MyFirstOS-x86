#ifndef __FS_DRIVER_H__
#define __FS_DRIVER_H__

#include "vfs/inode/inode.h"

struct superblock;
struct block_device;

typedef struct fs_driver 
{
    const char* name;

    bool (*match)(struct block_device* device);
    void (*mount)(struct superblock* fs_instance, struct block_device* block_dev);
    struct inode* (*read_inode)(struct superblock* sb, void* internal);
    struct inode* (*get_root_inode)(struct superblock* sb);

} fs_driver_t;

#endif // __FS_DRIVER_H__