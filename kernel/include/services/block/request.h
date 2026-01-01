#ifndef __BLOCK_REQUEST_H__
#define __BLOCK_REQUEST_H__

#include "core/num_defs.h"

typedef struct block_device block_device_t;

enum block_io_type
{
    BLOCK_IO_READ,
    BLOCK_IO_WRITE,
};

struct block_request;

typedef void (*block_request_cb)(struct block_request* request, i64 result);

typedef struct block_memchunk_entry
{
    void* pa_buffer;
    usize sectors;
} block_memchunk_entry_t;

typedef struct block_request
{
    block_device_t* device;
    enum block_io_type io;

    usize block_offset;
    usize block_count;

    void* block_vbuffer;  

    block_request_cb cb;
    void* ctx;

} block_request_t;

typedef void (*fn_block_submit_t)(
    block_request_t* request 
);

block_request_t* block_req_generate(
    block_device_t* device,
    enum block_io_type io,
    void* block_vbuffer,
    usize_ptr block_count,
    usize block_offset,
    block_request_cb cb,
    void* ctx
);

void block_req_cleanup(block_request_t* request);

#endif // __BLOCK_REQUEST_H__