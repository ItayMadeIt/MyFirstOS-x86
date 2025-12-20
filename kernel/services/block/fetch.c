#include "memory/virt/virt_alloc.h"
#include "services/block/manager.h"

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

#define MBR_SECTOR_SIZE 512

static void partition_add_block_device(block_device_t* disk_block_dev, const mbr_partition_t* partition)
{
    usize sector_multiplier = disk_block_dev->sector_size / MBR_SECTOR_SIZE;
    
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

static void fetch_partitions(block_device_t* block_dev)
{
    u8* page_data = kvalloc_pages(
        1, VREGION_BIO_BUFFER
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

void fetch_storage()
{
    usize stor_count = stor_get_device_count();
    for (usize_ptr i = 0; i < stor_count; i++)
    {
        block_device_t *device = block_disk_generate(stor_get_device(i));
        block_manager_add_device(device);

        fetch_partitions(device);
    }
}