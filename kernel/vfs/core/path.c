#include "vfs/core/path.h"
#include "core/defs.h"
#include "core/num_defs.h"
#include "string.h"
#include "vfs/core/dir_entry.h"
#include "vfs/core/dir_entry_cache.h"
#include "vfs/core/mount.h"
#include "vfs/inode/inode.h"

static void fetch_path_section(
    char res[MAX_NODE_NAME_LENGTH], 
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

    memcpy(res, &path[offset+1], size);
    res[size] = '\0';
}

static dir_entry_t* vfs_lookup(
    vfs_mount_map_t *vfs,
    dir_entry_t* parent, 
    const char* path, 
    usize_ptr offset)
{
    char cur_subpath[MAX_NODE_NAME_LENGTH + 1] = {0};
    fetch_path_section(cur_subpath, path, offset);

    dir_entry_t* entry =
        dir_entry_cache_lookup(&vfs->dcache, parent, cur_subpath);
    
    if (entry)
    {
        return entry;
    }

    assert(parent);
    assert(parent->inode);
    assert(parent->inode->ops);
    assert(parent->inode->ops->lookup);
    struct inode* inode = parent->inode->ops->lookup(parent->inode, cur_subpath);

    if (! inode)
    {
        return NULL;
    }

    entry = dir_entry_create(
        parent, 
        cur_subpath, 
        inode
    );

    dir_entry_cache_insert(&vfs->dcache, entry);

    return entry; 
}

dir_entry_t* vfs_lookup_path(
    vfs_mount_map_t* vfs,
    const char* path)
{
    if (!path || path[0] != '/')
        return NULL;

    if (path[1] == '\0')
        return vfs->root;

    
    usize_ptr offset = 0;
    dir_entry_t* cur = vfs->root;
    
    while (path[offset] != '\0')
    {
        dir_entry_t* child = vfs_lookup(vfs, cur, path, offset);
        if (!child)
            return NULL;

        dir_entry_t* mount_res = vfs_mount_resolve(vfs, child);
        if (mount_res)
            child = mount_res;

        iptr next = kfind_index_first_of_from(path, '/', offset + 1);
        if (next < 0)
            break;

        offset = (usize_ptr)next;
        cur = child;
    } 

    return cur;
}