#ifndef __BLOCK_DEVICE_H__
#define __BLOCK_DEVICE_H__

#include "services/block/request.h"
#include "services/block/types/types.h"

typedef struct block_device
{
    usize sector_size;
    usize sector_count;

    enum block_dev_type type;
    fn_block_submit_t submit;
    union block_dev_data data;

    char* vfs_name;

} block_device_t;

void block_submit(block_request_t* request);



#endif // __BLOCK_DEVICE_H__