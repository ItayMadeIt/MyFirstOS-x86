#include "vfs/core/vfs.h"
#include "memory/heap/heap.h"
#include "services/block/device.h"
#include "sys/num_defs.h"
#include "vfs/core/dir_entry_cache.h"
#include "vfs/core/drivers.h"
#include "vfs/core/mount.h"
#include "vfs/core/superblock.h"

typedef struct vfs_data
{
    vfs_mount_map_t mount;
} vfs_data_t;

static vfs_data_t vfs;

static bool init_root_bdev(block_device_t* block_dev)
{
    vfs_mount_init_tree(&vfs.mount);

    superblock_t* sb = kmalloc(sizeof(superblock_t));

    for (driver_count_t i = 0; i < fs_drivers_count; ++i) 
    {
        assert(fs_drivers[i]->match);
        if ( fs_drivers[i]->match(block_dev) )
        {
            assert(fs_drivers[i]->mount);
            fs_drivers[i]->mount(sb, block_dev);

            vfs_mount_set_root(
                &vfs.mount, 
                sb
            );

            return true;
        }
    }

    return false;
}

vfs_mount_map_t* vfs_get_mount_data()
{
    return &vfs.mount;
}

void init_vfs(block_device_t *block_dev)
{
    assert(init_root_bdev(block_dev));
}