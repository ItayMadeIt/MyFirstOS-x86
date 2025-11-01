#include "core/defs.h"
#include "core/num_defs.h"
#include "memory/heap/heap.h"
#include "services/storage/partition.h"
#include <services/storage/block_device.h>
#include <drivers/storage.h>
#include <kernel/devices/storage.h>
#include <stddef.h>

void init_arch_storage(storage_add_device add_func);

typedef struct stor_vars
{
    stor_device_t* dev_arr;
    usize_ptr capacity;
    usize_ptr count;
} stor_vars_t;

stor_vars_t storage;

static stor_device_t* main_device;
static u64 main_device_disk_size;

static usize_ptr add_stor_device(void* data, u64 sector_size, void (*submit)(stor_request_t*), u64 disk_size)
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
    storage.dev_arr[storage.count].submit = submit;
    storage.dev_arr[storage.count].disk_size = disk_size;

    u64 block_size = max(sector_size, PAGE_SIZE);
    storage.dev_arr[storage.count].cache.block_size = block_size;
    storage.dev_arr[storage.count].cache.pages_per_block = block_size / PAGE_SIZE;

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

stor_device_t *stor_get_device(u64 dev_index)
{
    if (dev_index >= storage.count)
    {
        return NULL;
    }

    return &storage.dev_arr[dev_index];
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
    storage.dev_arr = kalloc(sizeof(stor_device_t));
    
    assert(storage.dev_arr);

    init_arch_storage(add_stor_device);

    assert(main_device);

    for (u64 i = 0; i < storage.count; i++)
    {
        init_block_device(&storage.dev_arr[i]);
        device_scan_paritions(&storage.dev_arr[i]);
    }
}