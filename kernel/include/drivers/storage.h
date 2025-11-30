#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "core/num_defs.h"
#include "core/atomic_defs.h"
#include "services/threads/locks/spinlock.h"

typedef struct stor_device stor_device_t; 
typedef struct stor_request stor_request_t; 

typedef void (*stor_callback_t)(stor_request_t* request, i64 result);

typedef enum stor_request_io
{
    STOR_REQ_READ,
    STOR_REQ_WRITE,
} stor_request_io_t;

typedef struct stor_request_chunk_entry
{
    void* pa_buffer;
    u64 sectors;
} stor_request_chunk_entry_t;


typedef struct stor_request 
{
    stor_device_t* dev;

    usize lba;
    usize_ptr chunk_length;
    stor_request_chunk_entry_t* chunk_list;
    
    stor_request_io_t action;
    stor_callback_t callback;
    void* user_data;

} stor_request_t;

struct stor_device 
{
    usize dev_id;
    void* dev_data;
    usize disk_size;

    u32 active_requests;
    u32 max_requests;

    void (*submit)(stor_request_t* req);    

    usize sector_size;

    // lock not used by underlying stor_device like ide_device, but rather by the caller
    spinlock_t lock; 
};

stor_device_t* main_stor_device();

stor_device_t* stor_get_device(usize dev_index);
void init_storage();

typedef uptr (*storage_add_device)(
    void* data, 
    
    usize disk_size,
    usize sector_size, 
    
    void (*submit)(stor_request_t*), 

    u32 max_requests
);

#endif // __STORAGE_H__