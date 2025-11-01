#include "services/storage/partition.h"
#include "drivers/storage.h"
#include "memory/heap/heap.h"
#include "services/storage/block_device.h"
#include <stdio.h>

// MBR Defines
#define MBR_SECTOR_SIZE 512
#define MBR_OFF_PARTITIONS 446
#define MBR_PATITION_CONUT 4

typedef struct __attribute__((packed)) mbr_partition_entry {
    u8 bootFlag;
    u8 start_chs[3];
    u8 type;
    u8 end_chs[3];
    u32 start_lba;
    u32 sectors;
} mbr_partition_entry_t;

static void parse_mbr(stor_device_t* device, u8* mbr_buffer)
{
    u64 mbr_part_count = 0;
    for (u64 i = 0; i < MBR_PATITION_CONUT; i++)
    {
        mbr_partition_entry_t* entry = 
            (mbr_partition_entry_t*) &mbr_buffer[MBR_OFF_PARTITIONS + i * sizeof(mbr_partition_entry_t)];
        
        if (entry->type != 0)
        {
            mbr_part_count++;
        }
    }

    device->partition_table.count = mbr_part_count;
    device->partition_table.arr = kalloc(sizeof(stor_partition_t*) * device->partition_table.count);    
    assert(device->partition_table.arr);

    u64 cur_count = 0;

    for (u64 i = 0; i < MBR_PATITION_CONUT && cur_count < mbr_part_count; i++)
    {
        mbr_partition_entry_t* entry = 
            (mbr_partition_entry_t*) &mbr_buffer[MBR_OFF_PARTITIONS + i * sizeof(mbr_partition_entry_t)];
        
        if (entry->type == 0)
        {
            continue;
        }

        stor_partition_t* partition = kalloc(sizeof(stor_partition_t));
        assert(partition);

        assert(entry->sectors   * MBR_SECTOR_SIZE % device->cache.block_size == 0);
        assert(entry->start_lba * MBR_SECTOR_SIZE % device->cache.block_size == 0);
        partition->block_amount = entry->sectors * MBR_SECTOR_SIZE / device->cache.block_size;
        partition->block_lba    = entry->start_lba * MBR_SECTOR_SIZE / device->cache.block_size;
        
        partition->device = device;
        partition->index = cur_count;

        device->partition_table.arr[cur_count++] = partition;
    }
}

static void get_partitions(void* ctx, int status, cache_entry_t** entries, u64 count)
{
    assert(status >= 0);
    assert(count == 1);

    stor_device_t* device = ctx;

    u8* buffer = entries[0]->buffer;
    parse_mbr(device, buffer);
    
    stor_unpin_range_async(device, entries, 1, NULL, NULL);

    for (u32 i = 0; i < device->partition_table.count; i++)
    {
        printf("#%d\n - block_start: %X\n - block_amount: %X\n", 
            i, 
            (u32)device->partition_table.arr[i]->block_lba,
            (u32)device->partition_table.arr[i]->block_amount);
    }
}

void device_scan_paritions(stor_device_t *device)
{
    cache_entry_t** entries = kalloc(sizeof(cache_entry_t*) * 1);
    stor_pin_range_async(device, 0, 1, entries, get_partitions, device);
}