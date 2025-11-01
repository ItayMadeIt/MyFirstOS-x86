#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "core/num_defs.h"
#include <services/storage/block_device.h>
#include <services/storage/partition.h>
#include <utils/data_structs/flat_hashmap.h>

typedef struct stor_device stor_device_t; 
typedef struct stor_request stor_request_t; 

typedef void (*stor_callback_t)(stor_request_t* request, i64 result);

typedef enum stor_request_action 
{
    STOR_REQ_READ,
    STOR_REQ_WRITE,
} stor_request_action_t;

typedef struct stor_request_chunk_entry
{
    void* va_buffer;
    u64 sectors;
} stor_request_chunk_entry_t;


typedef struct stor_request 
{
    stor_device_t* dev;

    usize lba;
    usize_ptr chunk_length;
    stor_request_chunk_entry_t* chunk_list;
    
    stor_request_action_t action;
    stor_callback_t callback;
    void* user_data;

} stor_request_t;

struct stor_device 
{
    usize dev_id;
    void* dev_data;
    usize disk_size;

    void (*submit)(stor_request_t* req);    

    u64 sector_size;

    block_device_data_t cache;
    partition_table_t partition_table;
};

stor_device_t* main_stor_device();

stor_device_t* stor_get_device(u64 dev_index);
void init_storage();

typedef uptr (*storage_add_device)(
    void* data, 
    u64 sector_size, 
    void (*submit)(stor_request_t*), 
    u64 disk_size
);

static inline u64 cache_block_to_lba(stor_device_t *dev, u64 cache_index) 
{
    u64 byte_offset = cache_index * dev->cache.block_size;

    return byte_offset / dev->sector_size; 
}


#endif // __STORAGE_H__