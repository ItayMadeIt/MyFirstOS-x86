#include "core/defs.h"
#include "memory/heap/heap.h"
#include <services/storage/block_device.h>
#include <drivers/storage.h>
#include <kernel/devices/storage.h>
#include <stddef.h>

void init_arch_storage(storage_add_device add_func);

typedef struct stor_vars
{
    stor_device_t* dev_arr;
    uintptr_t capacity;
    uintptr_t count;
} stor_vars_t;

stor_vars_t storage;

static stor_device_t* main_device;
static uint64_t main_device_disk_size;

static uintptr_t add_stor_device(void* data, uint64_t sector_size, void (*submit)(stor_request_t*), uint64_t disk_size)
{
    assert(sector_size == align_up_pow2(sector_size));

    if (storage.count + 1 >= storage.capacity)
    {
        storage.capacity *= 2;
        storage.dev_arr = krealloc(storage.dev_arr, storage.capacity * sizeof(stor_device_t));
        assert(storage.dev_arr);
    }
    
    storage.dev_arr[storage.count].dev_data = data;
    storage.dev_arr[storage.count].dev_id = storage.count;
    storage.dev_arr[storage.count].sector_size = sector_size;
    storage.dev_arr[storage.count].sector_mask = ~(sector_size-1);
    storage.dev_arr[storage.count].submit = submit;
    storage.dev_arr[storage.count].disk_size = disk_size;

    uint64_t cache_size = max(sector_size, PAGE_SIZE);
    storage.dev_arr[storage.count].cache_size = cache_size;
    storage.dev_arr[storage.count].cache_mask = ~(cache_size-1);

    if (disk_size > main_device_disk_size)
    {
        main_device_disk_size = disk_size;
        main_device = &storage.dev_arr[storage.count];
    }

    return storage.count++;
}

stor_device_t* stor_main_device()
{
    return main_device;
}

stor_device_t *stor_get_device(uint64_t dev_index)
{
    if (dev_index >= storage.count)
    {
        return NULL;
    }

    return &storage.dev_arr[dev_index];
}

void init_storage()
{
    init_block_device();

    main_device_disk_size = 0;
    main_device = NULL;

    storage.capacity = 1;
    storage.count = 0;
    storage.dev_arr = kalloc(sizeof(stor_device_t));
    
    assert(storage.dev_arr);

    init_arch_storage(add_stor_device);

    assert(main_device);
}