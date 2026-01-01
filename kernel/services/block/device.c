#include "services/block/device.h"
#include "services/block/request.h"
#include "core/defs.h"

inline void block_submit(
    block_device_t* device,
    enum block_io_type io,
    void* block_vbuffer,
    usize_ptr block_count,
    usize block_offset,
    block_request_cb cb,
    void* ctx)
{
    assert(((uptr)block_vbuffer & (device->block_size-1)) == 0);

    block_request_t* request = block_req_generate(
        device, 
        io, 
        block_vbuffer, 
        block_count, 
        block_offset,
        cb, ctx
    );
    
    // make the device's submit layer handle actual logic (based on type)
    request->device->submit(request);
}