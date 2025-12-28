#ifndef __STORAGE_H__
#define __STORAGE_H__

#include "services/threads/locks/spinlock.h"
#include "utils/data_structs/ring_queue.h"

struct stor_device; 
struct stor_request; 

typedef void (*stor_callback_t)(struct stor_request* request, i64 result);

enum storage_dev_type 
{
    STOR_TYPE_PIO = 0,
    STOR_TYPE_DMA = 1,
};
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
    struct stor_device* device;

    usize lba;
    
    // will change to vbuffer and sector_count (I will assume DMA for now)

    usize_ptr chunk_length;
    stor_request_chunk_entry_t* chunk_list;
    
    stor_request_io_t action;
    stor_callback_t callback;
    void* ctx;

} stor_request_t;

typedef struct stor_device 
{
    usize dev_id;
    void* dev_data;

    usize disk_size;
    usize sector_size;

    u32 active_requests;
    u32 max_requests;

    void (*submit)(stor_request_t* req);    

    enum storage_dev_type type;

    // lock not used by underlying stor_device like ide_device, but rather by the caller
    spinlock_t lock; 
} stor_device_t;

struct stor_device* main_stor_device();

bool stor_submit(stor_request_t* request);

usize_ptr stor_get_device_count();
struct stor_device* stor_get_device(usize_ptr dev_index);
void init_storage();


typedef uptr (*storage_add_device)(
    void* data, 
    
    usize disk_size,
    usize sector_size, 
    
    void (*driver_submit)(stor_request_t*), 

    u32 max_requests,
    enum storage_dev_type type
);

#endif // __STORAGE_H__