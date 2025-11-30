#include "services/block/device.h"

inline void block_submit(block_request_t* request)
{
    // make the device's submit layer handle actual logic (based on type)
    request->device->submit(request);
}