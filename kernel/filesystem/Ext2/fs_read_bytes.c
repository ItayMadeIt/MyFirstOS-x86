#include "filesystem/drivers/Ext2/internal.h"
#include "kernel/core/cpu.h"
#include "memory/virt/virt_alloc.h"
#include "memory/virt/virt_region.h"
#include "services/block/device.h"
#include "services/block/request.h"
#include "core/defs.h"
#include <string.h>

static void ext2_read_cb(block_request_t* request, i64 result)
{
    assert(result >= 0);

    bool* done = request->ctx;
    *done = true;
}

void fs_read_bytes(
    superblock_t* sb, 
    void* data, usize_ptr length, 
    usize offset)
{
    bool done = false;

    usize unnecessary = offset % sb->device->block_size;

    usize fetch_length = length + unnecessary;

    usize_ptr pages = div_up(fetch_length, PAGE_SIZE);
    void* temp_vbuffer = kvalloc_pages(
        pages, 
        VREGION_BIO_BUFFER
    );

    usize block_count = align_up_n(fetch_length, sb->device->block_size)
        / sb->device->block_size;

    block_submit(
        sb->device, 
        BLOCK_IO_READ, 
        temp_vbuffer, 
        block_count, 
        offset / sb->device->block_size, 
        ext2_read_cb, &done
    );

    while (!done) cpu_relax();

    memcpy(
        data, 
        temp_vbuffer + unnecessary, 
        length
    );

    kvfree_pages(temp_vbuffer);
}
