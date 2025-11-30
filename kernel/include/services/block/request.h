#ifndef __BLOCK_REQUEST_H__
#define __BLOCK_REQUEST_H__

#include "core/num_defs.h"

typedef struct block_device block_device_t;

enum block_io_type
{
    BLOCK_IO_READ,
    BLOCK_IO_WRITE,
};

typedef struct block_request block_request_t;

typedef void (*block_request_cb)(block_request_t* request, void* ctx, i64 result);

typedef struct block_memchunk_entry
{
    void* pa_buffer;
    usize sectors;
} block_memchunk_entry_t;

typedef struct block_request
{
    block_device_t* device;
    enum block_io_type io;

    block_request_cb cb;
    void* ctx;

    usize lba;

    usize_ptr memchunks_count;
    usize_ptr memchunks_capacity;
    block_memchunk_entry_t memchunks[];

} block_request_t;

typedef void (*fn_block_submit_t)(
    block_request_t* request 
);

block_request_t* block_req_generate(
    block_device_t* device,
    enum block_io_type io,
    usize lba,
    usize memchunks_capacity,
    block_request_cb cb,
    void* ctx);

block_request_t* block_req_add_memchunk(
    block_request_t* request, 
    block_memchunk_entry_t memchunk_entry);

void block_req_cleanup(block_request_t* request);

#endif // __BLOCK_REQUEST_H__