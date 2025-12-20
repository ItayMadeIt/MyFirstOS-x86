#include "services/block/request.h"
#include "core/defs.h"
#include "core/assert.h"
#include "core/num_defs.h"
#include "memory/heap/heap.h"


block_request_t* block_req_generate(
    block_device_t* device,
    enum block_io_type io,
    usize lba,
    usize memchunks_capacity,
    block_request_cb cb,
    void* ctx)
{
    memchunks_capacity = align_up_pow2(memchunks_capacity);

    block_request_t* request = kmalloc(
        sizeof(block_request_t) + memchunks_capacity * sizeof(block_memchunk_entry_t)
    );
    assert(request);

    request->memchunks_capacity = memchunks_capacity;
    request->memchunks_count    = 0;

    request->device = device;
    request->io     = io;
    request->lba    = lba;
    request->cb     = cb;
    request->ctx    = ctx;

    return request;   
}

block_request_t* block_req_add_memchunk(
    block_request_t* request, 
    block_memchunk_entry_t memchunk_entry)
{
    assert(request);

    block_request_t* result = request;

    if (request->memchunks_count + 1 > request->memchunks_capacity)
    {
        usize_ptr new_capacity = request->memchunks_capacity * 2;

        block_request_t* new_request = krealloc(
            request, sizeof(block_request_t) + new_capacity * sizeof(block_memchunk_entry_t)
        );

        if (new_request == NULL)
            return NULL;

        result = new_request;
    }

    result->memchunks[result->memchunks_count++] = memchunk_entry;

    return result;
}

void block_req_cleanup(block_request_t* request)
{
    kfree(request);
}