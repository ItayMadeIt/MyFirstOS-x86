#include "vfs/core/path.h"
#include "core/defs.h"
#include "core/num_defs.h"
#include "string.h"
#include "vfs/core/defs.h"
#include "vfs/core/dir_entry.h"
#include "vfs/core/dir_entry_cache.h"
#include "vfs/core/errors.h"
#include "vfs/core/mount.h"
#include "vfs/inode/inode.h"
#include <string.h>

static bool fetch_path_section(
    char res[MAX_NODE_NAME_LENGTH+1], 
    const char* path, usize_ptr offset)
{
    assert(path[offset] == '/');
    iptr index = kfind_index_first_of_from(path, '/', offset + 1);

    usize_ptr size;
    if (index < 0)
    {
        index = strlen(path);
    }

    size = index - (offset+1);

    if (size > MAX_NODE_NAME_LENGTH+1)
    {
        return false;
    }

    memcpy(res, &path[offset+1], size);
    res[size] = '\0';

    return true;
}

static i32 path_lookup_step(
    vfs_mount_map_t *vfs,
    dir_entry_t* parent, 
    const char* path, 
    usize_ptr offset,
    dir_entry_t** child)
{
    char cur_subpath[MAX_NODE_NAME_LENGTH + 1] = {0};
    bool fetched = fetch_path_section(cur_subpath, path, offset);

    if (! fetched)
    {
        return -VFS_ERR_NAMETOOLONG;
    }

    dir_entry_t* entry =
        dir_entry_cache_lookup(&vfs->dcache, parent, cur_subpath);
    
    if (entry)
    {
        *child = entry;
        return 0;
    }

    assert(parent);
    assert(parent->inode);
    assert(parent->inode->ops);
    assert(parent->inode->ops->lookup);

    struct inode* inode;
    i32 result = parent->inode->ops->lookup(parent->inode, cur_subpath, &inode);

    if (result < 0)
    {
        return result;
    }

    entry = dir_entry_create(
        parent, 
        cur_subpath, 
        inode
    );

    dir_entry_cache_insert(&vfs->dcache, entry);

    *child = entry;
    return 0; 
}

i32 vfs_lookup_path(
    vfs_mount_map_t* vfs,
    const char* path,
    dir_entry_t** result)
{
    assert(result);

    if (!path || path[0] != '/')
        return -VFS_ERR_INVAL;

    if (path[1] == '\0')
    {
        *result = vfs->root;
        return 0;
    }

    
    usize_ptr offset = 0;
    dir_entry_t* cur = vfs->root;
    
    while (path[offset] != '\0')
    {
        dir_entry_t* child;
        i32 result = path_lookup_step(vfs, cur, path, offset, &child);
        if (!child || result < 0)
            return result;

        dir_entry_t* mount_res = vfs_mount_resolve(vfs, child);
        if (mount_res)
            child = mount_res;

        cur = child;

        iptr next = kfind_index_first_of_from(path, '/', offset + 1);
        if (next < 0)
            break;

        offset = (usize_ptr)next;
    } 

    *result = cur;
    return 0;
}