#ifndef __VFS_ERRORS_H__
#define __VFS_ERRORS_H__

enum vfs_error
{
    VFS_OK             = 0,     // success

    VFS_ERR_NOENT      = -1,   // no such file or directory
    VFS_ERR_EXIST      = -2,   // file already exists
    VFS_ERR_NOTDIR     = -3,   // not a directory
    VFS_ERR_ISDIR      = -4,   // is a directory
    VFS_ERR_PERM       = -5,   // permission denied
    VFS_ERR_NOMEM      = -6,   // out of memory
    VFS_ERR_INVAL      = -7,   // invalid argument
    VFS_ERR_IO         = -8,   // I/O error
    VFS_ERR_ROFS       = -9,   // read-only filesystem
    VFS_ERR_NOSPC      = -10,  // no space left
    VFS_ERR_OPNOTSUPP  = -11,  // operation not supported
    VFS_ERR_NOTEMPTY   = -12,  // directory not empty
    VFS_ERR_BUSY       = -13,  // resource busy (e.g., mount point)
    VFS_ERR_NAMETOOLONG= -14,  // file name too long
}; // copied errors, see how usefull they yare

#endif // __VFS_ERRORS_H__