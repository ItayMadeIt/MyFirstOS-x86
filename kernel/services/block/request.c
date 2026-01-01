#include "services/block/request.h"
#include "core/defs.h"
#include "core/assert.h"
#include "core/num_defs.h"
#include "memory/heap/heap.h"


block_request_t* block_req_generate(
    block_device_t* device,
    enum block_io_type io,
    void* block_vbuffer,
    usize_ptr block_count,
    usize block_offset,
    block_request_cb cb,
    void* ctx)
{
    block_request_t* request = kmalloc(sizeof(block_request_t));
    assert(request);

    request->cb            = cb;
    request->ctx           = ctx;
    request->block_offset  = block_offset;
    request->block_count  = block_count;
    request->block_vbuffer = block_vbuffer;
    request->io            = io;
    request->device        = device;

    return request;   
}

void block_req_cleanup(block_request_t* request)
{
    kfree(request);
}