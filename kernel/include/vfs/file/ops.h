#ifndef __VFS_OPS_H__
#define __VFS_OPS_H__

#include "core/num_defs.h"

struct file;

typedef struct file_ops
{
    i32 (*read )(struct file* file, u8* buffer, u32 count);
    i32 (*write)(struct file* file, const u8* buffer, u32 count);
} file_ops_t;

#endif // __VFS_OPS_H__