#include "memory/core/pfn_desc.h"
#include "core/assert.h"
#include "memory/virt/virt_alloc.h"
#include "services/block/device.h"
#include "services/block/fetch.h"

// will change to a dynamic driver count
#define MAX_DEVS 128
static block_device_t* block_devs[MAX_DEVS];
static usize_ptr       block_dev_count = 0;

static usize           block_dev_max_size = 0;
static block_device_t* main_block_dev = NULL;

void block_manager_add_device(block_device_t* device)
{
    block_devs[block_dev_count++] = device;

    if (device->sector_size * device->sector_count > block_dev_max_size)
    {
        block_dev_max_size = device->sector_size * device->sector_count;
        main_block_dev = device; 
    }
}

block_device_t* block_manager_get_device(usize_ptr index)
{
    return block_devs[index];
}

block_device_t* block_manager_main_device()
{
    return main_block_dev;
}

void init_block_manager()
{
    fetch_storage();
}