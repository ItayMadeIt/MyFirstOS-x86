#include "core/defs.h"
#include "core/num_defs.h"
#include "memory/heap/heap.h"
#include <drivers/storage.h>
#include <kernel/devices/storage.h>
#include <stddef.h>

void init_arch_storage(storage_add_device add_func);

typedef struct stor_vars
{
    stor_device_t** dev_arr;
    usize_ptr capacity;
    usize_ptr count;
} stor_vars_t;

static stor_vars_t storage;

static stor_device_t* main_device;
static u64 main_device_disk_size;

bool stor_submit(stor_request_t* request)
{
    bool success = false;

    stor_device_t* dev = request->device;
    spinlock_lock(&dev->lock);

    // should check in the future that the hardware agrees this is a valid way to use it
    // (check sector size, valid DMA bfufer, all of those)


    if (dev->active_requests < dev->max_requests) 
    {
        dev->submit(request);
        success = true;
    }

    spinlock_unlock(&dev->lock);

    return success;
}

static usize_ptr add_stor_device(
    void* data, 
    usize disk_size, usize sector_size, 
    void (*submit)(stor_request_t*), 
    u32 max_requests, 
    enum storage_dev_type type)
{
    assert(sector_size == align_up_pow2(sector_size));

    if (storage.count + 1 >= storage.capacity)
    {
        storage.capacity *= 2;
        storage.dev_arr = krealloc(storage.dev_arr, storage.capacity * sizeof(stor_device_t*));
        assert(storage.dev_arr);
    }
    
    stor_device_t* cur_device = kmalloc(sizeof(stor_device_t));
    assert(cur_device);

    spinlock_initlock(&cur_device->lock, false);
 
    *cur_device = (stor_device_t){
        .max_requests    = max_requests,
        .active_requests = 0, // or use atomic_init later
        .dev_data        = data,
        .dev_id          = storage.count,
        .sector_size     = sector_size,
        .submit          = submit,
        .disk_size       = disk_size,
        .type            = type,
        .lock            = cur_device->lock, // preserve the spinlock state
    };

    storage.dev_arr[storage.count] = cur_device;

    if (disk_size > main_device_disk_size)
    {
        main_device_disk_size = disk_size;
        main_device = storage.dev_arr[storage.count];
    }

    return storage.count++;
}

stor_device_t* stor_main_device()
{
    return main_device;
}

usize_ptr stor_get_device_count()
{
    return storage.count;
}

stor_device_t *stor_get_device(usize_ptr dev_index)
{
    if (dev_index >= storage.count)
    {
        return NULL;
    }

    return storage.dev_arr[dev_index];
}
stor_device_t* main_stor_device()
{
    return main_device;
}

void init_storage()
{
    main_device_disk_size = 0;
    main_device = NULL;

    storage.capacity = 1;
    storage.count = 0;
    storage.dev_arr = kmalloc(sizeof(stor_device_t*));
    assert(storage.dev_arr);

    init_arch_storage(add_stor_device);

    assert(main_device);
}