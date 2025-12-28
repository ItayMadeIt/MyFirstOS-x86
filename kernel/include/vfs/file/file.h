#ifndef __FILE_H__
#define __FILE_H__

#include "vfs/core/defs.h"
#include "vfs/inode/inode.h"

typedef struct file 
{
    inode_t* inode;
    off_t offset;
    flags_t flags;

    
} file_t;

#endif // __FILE_H__