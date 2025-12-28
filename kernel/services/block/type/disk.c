#include "services/block/types/disk.h"
#include "arch/i386/memory/paging_utils.h"
#include "core/defs.h"
#include "core/num_defs.h"
#include "drivers/storage.h"
#include "memory/heap/heap.h"
#include "memory/virt/virt_alloc.h"
#include "memory/virt/virt_region.h"
#include "services/block/request.h"
#include "services/block/device.h"
#include "services/block/types/types.h"
#include "utils/data_structs/ring_queue.h"
#include <string.h>

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
    result->device = hw_device;
    
    result->chunk_length = 1;
    result->chunk_list   = kmalloc(sizeof(stor_request_chunk_entry_t));
    assert(result->chunk_list);

    result->ctx = block_request;

    if (((uptr)block_request->vbuffer & (hw_device->sector_size - 1)) == 0 && 
        ((uptr)block_request->offset  & (hw_device->sector_size - 1)) == 0 && 
        ((uptr)block_request->length  & (hw_device->sector_size - 1)) == 0)
    {
        block_request->bounce_vbuffer = NULL;

        result->chunk_list->pa_buffer = virt_to_phys(block_request->vbuffer);
        result->chunk_list->sectors   = block_request->length / hw_device->sector_size;

        result->lba = block_request->offset / hw_device->sector_size;
    }
    else 
    {
        assert(PAGE_SIZE >= hw_device->sector_size);

        usize_ptr forced_bytes = block_request->offset & (hw_device->sector_size - 1);
        usize_ptr full_length  = block_request->length + forced_bytes;

        usize_ptr pages_count =
            align_up_n(full_length, PAGE_SIZE) / PAGE_SIZE;

        block_request->bounce_vbuffer = kvalloc_pages(
            pages_count,
            VREGION_BIO_BUFFER
        );

        result->chunk_list->pa_buffer = virt_to_phys(block_request->bounce_vbuffer);
        result->chunk_list->sectors   = 
            align_up_n(full_length, hw_device->sector_size)/ hw_device->sector_size;

        result->lba = block_request->offset / hw_device->sector_size;
    }
    
    return result;
}



static void stor_disk_cb(stor_request_t* stor_request, i64 result)
{
    assert(result >= 0);

    stor_device_t*    stor_dev       = stor_request->device;
    block_request_t*  block_request = (block_request_t*) stor_request->ctx;
    block_device_t*   block_dev      = block_request->device;
    block_dev_disk_t* disk_data      = &block_dev->data.disk;
    ring_queue_t*     queue          = &disk_data->queue;

    if (block_request->bounce_vbuffer)
    {
        usize_ptr forced_bytes = block_request->offset & (stor_dev->sector_size - 1);
        void* begin_va_buffer = (void*)((uptr)block_request->bounce_vbuffer + forced_bytes);
        memcpy(block_request->vbuffer, begin_va_buffer, block_request->length);

        kvfree_pages(block_request->bounce_vbuffer);
        block_request->bounce_vbuffer = NULL;
    }
    block_request->cb(block_request, result);

    spinlock_lock(&stor_dev->lock);
    if (stor_dev->active_requests < stor_dev->max_requests && 
        !ring_queue_is_empty(queue))
    {
        block_request_t* next = ring_queue_pop(queue);
        stor_request_t* next_stor_request = block_disk_make_stor_request(next);
        assert( stor_submit(next_stor_request) ) ;
    }
    spinlock_unlock(&stor_dev->lock);

    kfree(stor_request->chunk_list);
    kfree(stor_request);
}

static void block_submit_disk(block_request_t* block_request)
{
    block_dev_disk_t* disk_data = &block_request->device->data.disk;
    stor_device_t* hw_device = disk_data->hw_device;
    
    ring_queue_t* queue = &disk_data->queue;

    spinlock_lock(&hw_device->lock);
    bool can_add_request = hw_device->active_requests < hw_device->max_requests;
    spinlock_unlock(&hw_device->lock);
    
    if (can_add_request)
    {
        stor_request_t* stor_request = block_disk_make_stor_request(block_request);        
        stor_submit(stor_request);
    }
    else 
    {
        ring_queue_push(queue, block_request);
    }

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