#ifndef __PARTITION_H__
#define __PARTITION_H__

#include "core/num_defs.h"

typedef struct stor_device stor_device_t;


typedef struct __attribute__((packed)) stor_partition_t 
{
    stor_device_t* device;
    usize index; 

    usize block_lba;
    usize block_amount;

} stor_partition_t;

typedef struct partition_table
{
    stor_partition_t** arr;
    usize count;
} partition_table_t;

stor_partition_t* get_partition(stor_device_t* device, usize index);
void device_scan_paritions(stor_device_t* device);

#endif // __PARTITION_H__