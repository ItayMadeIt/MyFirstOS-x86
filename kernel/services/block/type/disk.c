#include "services/block/types/disk.h"
#include "core/num_defs.h"
#include "drivers/storage.h"
#include "memory/heap/heap.h"
#include "services/block/request.h"
#include "services/block/device.h"
#include "services/block/types/types.h"
#include "utils/data_structs/ring_queue.h"

static void stor_disk_cb(stor_request_t* stor_request, i64 result);

inline static enum stor_request_io block_to_stor_type(enum block_io_type block_type)
{
    switch (block_type)
    {
        case BLOCK_IO_READ:
            return STOR_REQ_READ;
        case BLOCK_IO_WRITE:
            return STOR_REQ_WRITE;
        default:
            assert(false);
    }
}

static stor_request_t* block_disk_make_stor_request(block_request_t* block_request)
{
    stor_request_t* result = kmalloc(sizeof(stor_request_t));
    assert(result);

    block_dev_disk_t* disk_data = &block_request->device->data.disk;
    stor_device_t* hw_device = disk_data->hw_device;

    result->action = block_to_stor_type(block_request->io);
    result->callback = stor_disk_cb;
    result->dev = hw_device;
    result->lba = block_request->lba;

    result->chunk_length = block_request->memchunks_count;
    result->chunk_list   = kmalloc(result->chunk_length * sizeof(stor_request_chunk_entry_t));
    assert(result->chunk_list);

    for (usize_ptr i = 0; i < result->chunk_length; ++i) 
    {
        result->chunk_list[i].pa_buffer = block_request->memchunks[i].pa_buffer;
        result->chunk_list[i].sectors   = block_request->memchunks[i].sectors;
    }
    
    result->user_data = block_request;

    return result;
}



static void stor_disk_cb(stor_request_t* stor_request, i64 result)
{
    assert(result >= 0);

    stor_device_t*    stor_dev      = stor_request->dev;
    block_request_t*  block_request = (block_request_t*) stor_request->user_data;
    block_device_t*   block_dev     = block_request->device;
    block_dev_disk_t* disk_data     = &block_dev->data.disk;
    ring_queue_t*     queue         = &disk_data->queue;

    spinlock_lock(&stor_dev->lock);
    stor_dev->active_requests--;

    if (stor_dev->active_requests < stor_dev->max_requests && 
        !ring_queue_is_empty(queue))
    {
        block_request_t* next = ring_queue_pop(queue);
        stor_request_t* next_stor_request = block_disk_make_stor_request(next);
        stor_dev->submit(next_stor_request);
        stor_dev->active_requests++;
    }
    spinlock_unlock(&stor_dev->lock);

    block_request->cb(block_request, block_request->ctx, result);

    kfree(stor_request->chunk_list);
    kfree(stor_request);
}

static void block_submit_disk(block_request_t* block_request)
{
    block_dev_disk_t* disk_data = &block_request->device->data.disk;
    stor_device_t* hw_device = disk_data->hw_device;
    
    ring_queue_t* queue = &disk_data->queue;

    spinlock_lock(&hw_device->lock);
    
    if (hw_device->active_requests < hw_device->max_requests)
    {
        stor_request_t* stor_request = block_disk_make_stor_request(block_request);
        
        hw_device->submit(stor_request);

        hw_device->active_requests++;
    }
    else 
    {
        ring_queue_push(queue, block_request);
    }

    spinlock_unlock(&hw_device->lock);
}

block_device_t* block_disk_generate(stor_device_t* stor_dev)
{
    block_device_t* block_device = kmalloc(sizeof(block_device_t));

    block_device->submit = block_submit_disk;
    block_device->type = BLOCK_DEV_DISK;

    block_device->sector_size  = stor_dev->sector_size;
    block_device->sector_count = stor_dev->disk_size / stor_dev->sector_size;

    block_device->data.disk.hw_device = stor_dev;
    ring_queue_init(&block_device->data.disk.queue);
    
    return block_device;
}