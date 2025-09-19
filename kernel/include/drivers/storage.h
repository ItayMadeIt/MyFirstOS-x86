#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdint.h>

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
   
    uintptr_t sector_size;
    uintptr_t sector_mask;
};

stor_device_t* stor_get_device(uint64_t dev_index);
void init_storage();

typedef uintptr_t (*storage_add_device)(
    void* data, 
    uintptr_t sector_size, 
    void (*submit)(stor_request_t*), 
    uint64_t disk_size
);

#endif // __STORAGE_H__