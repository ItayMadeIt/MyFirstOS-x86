#ifndef __VFS_DIR_ENRTY_H__
#define __VFS_DIR_ENRTY_H__

#include "vfs/inode/inode.h"

typedef struct dir_entry 
{
    struct dir_entry* parent;
    struct inode* inode;

    char name[];

} dir_entry_t;

#endif // __DIR_ENRTY_H__