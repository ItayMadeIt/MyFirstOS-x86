#include "utils/data_structs/ring_queue.h"
#include "core/num_defs.h"
#include "memory/heap/heap.h"
#include <string.h>

#define RING_QUEUE_INIT_CAPACITY 16

void ring_queue_init(ring_queue_t* queue)
{
    queue->arr = kmalloc(sizeof(void*) * RING_QUEUE_INIT_CAPACITY);
    assert(queue->arr);

    queue->head    = 0;
    queue->count    = 0;
    queue->capacity = RING_QUEUE_INIT_CAPACITY;
}

void ring_queue_init_capacity(ring_queue_t* queue, usize_ptr init_capacity)
{
    // init capacity = 2^n
    assert((init_capacity & (init_capacity - 1)) == 0);

    queue->arr = kmalloc(sizeof(void*) * init_capacity);
    assert(queue->arr);

    queue->head       = 0;
    queue->count       = 0;
    queue->capacity    = init_capacity;
}

static inline usize_ptr ring_queue_get_index(ring_queue_t* queue, usize_ptr offset)
{
    return (queue->head + offset) & (queue->capacity - 1); 
}

static bool ring_queue_grow(ring_queue_t* queue)
{
    usize_ptr head = queue->head;
    usize_ptr capacity = queue->capacity;
    usize_ptr count = queue->count;
    void** arr = queue->arr;

    usize_ptr start_arr_item_count = count - (capacity - head);

    usize_ptr new_capacity = capacity * 2;
    void** new_arr = kmalloc(new_capacity * sizeof(void*));
    memcpy(new_arr, arr, start_arr_item_count * sizeof(void*));

    usize_ptr new_head = head + capacity;

    memmove(
        &new_arr[new_head],  
        &new_arr[head], 
        min(capacity - head, count - head)
    );

    // this thread resizes (avoid race condition)
    if (queue->capacity < new_capacity) 
    {
        queue->arr = new_arr;
        queue->head = new_head;
        queue->capacity = new_capacity;
    }    
    else
    {
        kfree(new_arr);
    }
    
    return true;
}

static bool ring_queue_ensure_size(ring_queue_t* queue, usize_ptr add_count)
{
    usize count = queue->count;
    usize capacity = queue->capacity;

    if (count + add_count > capacity)
        return ring_queue_grow(queue);

    return true;
}

bool ring_queue_push(ring_queue_t* queue, void* item)
{
    if (!ring_queue_ensure_size(queue, 1))
        return false;

    usize_ptr relative_index = ring_queue_get_index(queue, queue->count);

    queue->arr[relative_index] = item;
    queue->count++;

    return true;
}

void* ring_queue_pop(ring_queue_t* queue)
{
    if (queue->count == 0)
    {
        return NULL;
    }

    void* item = queue->arr[queue->head];

    queue->head = ring_queue_get_index(queue, 1);
    queue->count--;

    return item;
}


bool ring_queue_is_empty(ring_queue_t* queue)
{
    return queue->count == 0;
}