#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdint.h>
#include <services/storage/block_device.h>
#include <utils/data_structs/flat_hashmap.h>

typedef struct stor_device stor_device_t; 
typedef struct stor_request stor_request_t; 

typedef void (*stor_callback_t)(stor_request_t* request, int64_t result);

typedef enum stor_request_action 
{
    STOR_REQ_READ,
    STOR_REQ_WRITE,
} stor_request_action_t;

typedef struct stor_request 
{
    stor_device_t* dev;

    uint64_t lba;
    uint64_t sectors;

    void* va_buffer;
    
    stor_request_action_t action;

    stor_callback_t callback;

    void* user_data;

} stor_request_t;

struct stor_device 
{
    uint64_t dev_id;
    void* dev_data;
    uint64_t disk_size;

    void (*submit)(stor_request_t* req);    
   
    uint64_t sector_size;
    uint64_t sector_mask;
    
    uint64_t block_size; // max(page_size, sector_size)
    uint64_t cache_mask; 

    block_device_data_t cache;
};

stor_device_t* main_stor_device();

stor_device_t* stor_get_device(uint64_t dev_index);
void init_storage();

typedef uintptr_t (*storage_add_device)(
    void* data, 
    uint64_t sector_size, 
    void (*submit)(stor_request_t*), 
    uint64_t disk_size
);

static inline uint64_t cache_block_to_lba(stor_device_t *dev, uint64_t cache_index) 
{
    uint64_t byte_offset = cache_index * dev->block_size;

    return byte_offset / dev->sector_size; 
}


#endif // __STORAGE_H__