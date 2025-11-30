#include "arch/i386/core/paging.h"
#include "arch/i386/memory/paging_utils.h"
#include "core/defs.h"
#include "core/num_defs.h"
#include "kernel/memory/paging.h"
#include "memory/core/virt_alloc.h"
#include "memory/heap/heap.h"
#include "services/block/device.h"
#include "services/block/manager.h"
#include "services/block/request.h"
#include "services/block/types/partition.h"
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

stor_vars_t storage;

static stor_device_t* main_device;
static u64 main_device_disk_size;

static usize_ptr add_stor_device(
    void* data, 
    usize disk_size, usize sector_size, 
    void (*submit)(stor_request_t*), 
    u32 max_requests)
{
    assert(sector_size == align_up_pow2(sector_size));

    if (storage.count + 1 >= storage.capacity)
    {
        storage.capacity *= 2;
        storage.dev_arr = krealloc(storage.dev_arr, storage.capacity * sizeof(stor_device_t*));
        assert(storage.dev_arr);
    }
    
    stor_device_t* cur_device = kalloc(sizeof(stor_device_t));
    assert(cur_device);

    spinlock_initlock(&cur_device->lock, false);
 
    *cur_device = (stor_device_t){
        .max_requests     = max_requests,
        .active_requests  = 0, // or use atomic_init later
        .dev_data         = data,
        .dev_id           = storage.count,
        .sector_size      = sector_size,
        .submit           = submit,
        .disk_size        = disk_size,
        .lock             = cur_device->lock, // preserve the spinlock state
    };

    cur_device->dev_data = data;
    cur_device->dev_id = storage.count;
    cur_device->sector_size = sector_size;
    cur_device->submit = submit;
    cur_device->max_requests = 1;
    cur_device->disk_size = disk_size;
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

stor_device_t *stor_get_device(usize dev_index)
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

#define PARTITIONS_AMOUNT 4 
typedef struct __attribute__((packed)) mbr_partition 
{
    u8  status;
    u8  chs_first[3];
    u8  type;
    u8  chs_last[3];
    u32 lba_offset;
    u32 sector_count;
} mbr_partition_t;


typedef struct __attribute__((packed)) mbr 
{
    u8        boot[446];
    mbr_partition_t partitions[PARTITIONS_AMOUNT];
    u16       signature;
} mbr_t;


static void partition_add_block_device(block_device_t* disk_block_dev, const mbr_partition_t* partition)
{
    usize sector_multiplier = disk_block_dev->sector_size / SECTOR_SIZE;
    
    usize lba_offset = partition->lba_offset / sector_multiplier;
    usize sector_count = partition->sector_count / sector_multiplier;
    
    block_device_t* partition_dev = block_partition_generate(disk_block_dev,lba_offset, sector_count);
    block_manager_add_device(partition_dev);
}

static bool parse_mbr(block_device_t* block_dev, const uint8_t* page_data) 
{
    const mbr_t* mbr = (const mbr_t*)page_data;

    if (mbr->signature != 0xAA55) 
    {
        return false;
    }

    for (int i = 0; i < 4; i++) 
    {
        if (mbr->partitions[i].type == 0)
        {
            continue;
        }

        partition_add_block_device(block_dev, &mbr->partitions[i]);
    }

    return true;
}


static void partitions_cb(block_request_t* request, void* ctx, i64 result)
{
    (void)request;
    assert(result >= 0);

    u8* page_data = ctx;

    parse_mbr(request->device, page_data);

    block_req_cleanup(request);

    kvfree_pages(page_data);
}

static void partitions_gather(block_device_t* block_dev)
{
    u8* page_data = kvalloc_pages(
        1, 
        VREGION_CACHE_CLONE, 
        VIRT_PHYS_FLAG_WRITE | VIRT_PHYS_FLAG_NOEXEC
    );

    block_request_t* request = block_req_generate(
        block_dev, BLOCK_IO_READ, 
        0, 1, 
        partitions_cb, page_data
    );

    request = block_req_add_memchunk(
        request, 
        (block_memchunk_entry_t) {
            .pa_buffer=virt_to_phys(page_data),
            .sectors=PAGE_SIZE/block_dev->sector_size
        }
    );

    block_submit(request);
}

void init_storage()
{
    main_device_disk_size = 0;
    main_device = NULL;

    storage.capacity = 1;
    storage.count = 0;
    storage.dev_arr = kalloc(sizeof(stor_device_t*));
    assert(storage.dev_arr);

    init_arch_storage(add_stor_device);

    assert(main_device);

    for (usize_ptr i = 0; i < storage.count; i++)
    {
        block_device_t *device = block_disk_generate(storage.dev_arr[i]);
        block_manager_add_device(device);

        partitions_gather(device);
    }
}