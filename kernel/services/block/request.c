#include "services/block/request.h"
#include "core/defs.h"
#include "core/assert.h"
#include "core/num_defs.h"
#include "memory/heap/heap.h"


block_request_t* block_req_generate(
    block_device_t* device,
    enum block_io_type io,
    void* vbuffer,
    usize_ptr length,
    usize offset,
    block_request_cb cb,
    void* ctx)
{
    block_request_t* request = kmalloc(sizeof(block_request_t));
    assert(request);

    request->bounce_vbuffer = NULL;
    request->cb = cb;
    request->ctx = ctx;
    request->offset = offset;
    request->length = length;
    request->vbuffer = vbuffer;
    request->io = io;
    request->device = device;

    return request;   
}

void block_req_cleanup(block_request_t* request)
{
    kfree(request);
}