#ifndef __PARTITION_H__
#define __PARTITION_H__

#include <stdint.h>

typedef struct stor_device stor_device_t;


typedef struct __attribute__((packed)) stor_partition_t 
{
    stor_device_t* device;
    uint64_t index; 

    uint64_t block_lba;
    uint64_t block_amount;

} stor_partition_t;

typedef struct partition_table
{
    stor_partition_t** arr;
    uint64_t count;
} partition_table_t;

stor_partition_t* get_partition(stor_device_t* device, uint64_t index);
void device_scan_paritions(stor_device_t* device);

#endif // __PARTITION_H__