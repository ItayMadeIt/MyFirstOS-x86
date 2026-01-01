#include "filesystem/drivers/Ext2/features.h"
#include "core/num_defs.h"
#include "filesystem/drivers/Ext2/disk.h"
#include "filesystem/drivers/Ext2/inode.h"
#include "filesystem/drivers/Ext2/internal.h"
#include "memory/heap/heap.h"
#include "vfs/core/errors.h"
#include "vfs/inode/inode_cache.h"
#include "core/defs.h"
#include <string.h>

inode_t* ext2_get_root_inode(superblock_t* sb)
{
    inode_t* root_inode = ext2_fetch_inode(sb, EXT2_ROOT_INODE);
    assert(root_inode);
    return root_inode;
}

i32 ext2_vfs_read(struct inode* inode, void* vbuffer, usize size, off_t offset)
{
    ext2_inode_t* ext2_inode = inode->fs_internal;
    
    if (ext2_inode_is_type(&ext2_inode->disk, EXT2_FT_DIR))
    {
        return -VFS_ERR_ISDIR;
    }

    return ext2_read_inode(
        inode,
        vbuffer,
        size,
        offset
    );
}

i32 ext2_vfs_lookup(
    struct inode* inode, 
    const char* child_str, 
    struct inode** child_ptr)
{
    assert(child_ptr);

    ext2_inode_t* ext2_inode = inode->fs_internal;
    if (! ext2_inode_is_type(&ext2_inode->disk, EXT2_FT_DIR)) 
    {
        return -VFS_ERR_NOTDIR;
    }

    u8* dir_data = kmalloc(ext2_inode->disk.size);
    i32 res = ext2_read_inode(
        inode, 
        dir_data, 
        ext2_inode->disk.size, 
        0
    );
    if (res < 0)
    {
        return res;
    }

    usize_ptr child_str_length = strlen(child_str);

    ext2_disk_dir_entry_t* disk_entry = (ext2_disk_dir_entry_t*) dir_data;
    u32 cur = 0;
    while (cur < ext2_inode->disk.size)
    {
        if (child_str_length == disk_entry->name_len &&
            memcmp(disk_entry->name, child_str, disk_entry->name_len) == 0)
        {
            *child_ptr = ext2_fetch_inode(inode->sb, disk_entry->inode);
            kfree(dir_data);
            return 0;
        }
        cur += disk_entry->rec_len;

        disk_entry = (ext2_disk_dir_entry_t*)(dir_data + cur);
    }
    kfree(dir_data);
    return -VFS_ERR_NOENT;
}