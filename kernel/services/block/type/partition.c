#include "services/block/types/partition.h"
#include "core/num_defs.h"
#include "memory/heap/heap.h"
#include "services/block/device.h"
#include "services/block/request.h"
#include "services/block/types/types.h"
#include "core/defs.h" // IWYU pragma: keep (clang hates me)

static void block_disk_cb(block_request_t* disk_request, void* ctx, i64 result)
{
    assert(result >= 0);

    kfree(disk_request);

    block_request_t* user_request = ctx;
    
    user_request->cb(user_request, user_request->ctx, result);
}

static void block_submit_partition(block_request_t* user_request)
{
    block_dev_partition_t* partition = &user_request->device->data.partition;
    block_device_t* disk_block_device = partition->disk_block_device;

    block_request_t* disk_request = block_req_generate(
        disk_block_device, 
        user_request->io, 
        user_request->lba + partition->lba_offset, 
        user_request->memchunks_count, 
        block_disk_cb, 
        user_request
    );
    
    for (usize_ptr i = 0; i < user_request->memchunks_count; i++) 
    {    
        disk_request = block_req_add_memchunk(
            disk_request, 
            user_request->memchunks[i]
        ); 
    }

    block_submit(disk_request);
}

block_device_t* block_partition_generate( 
    block_device_t* disk_block_dev,
    usize lba_offset, usize sector_count)
{
    assert(disk_block_dev->type == BLOCK_DEV_DISK);

    block_device_t* partition_block_dev = kmalloc(sizeof(block_device_t));

    partition_block_dev->submit = block_submit_partition;
    partition_block_dev->type = BLOCK_DEV_PARTITION;

    partition_block_dev->sector_size  = disk_block_dev->sector_size;
    partition_block_dev->sector_count = sector_count / disk_block_dev->sector_size;

    partition_block_dev->data.partition.disk_block_device = disk_block_dev;
    partition_block_dev->data.partition.lba_offset = lba_offset;
    
    return partition_block_dev;
}