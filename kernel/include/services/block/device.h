#ifndef __BLOCK_DEVICE_H__
#define __BLOCK_DEVICE_H__

#include "services/block/request.h"
#include "services/block/types/types.h"

typedef struct block_device
{
    usize block_size;
    usize block_count;

    enum block_dev_type type;
    fn_block_submit_t submit;
    union block_dev_data data;

    char* vfs_name;

} block_device_t;

void block_submit(
    block_device_t* device,
    enum block_io_type io,
    void* block_vbuffer,
    usize_ptr block_count,
    usize offset,
    block_request_cb cb,
    void* ctx
);



#endif // __BLOCK_DEVICE_H__