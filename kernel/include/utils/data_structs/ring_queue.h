#ifndef __UTILS_RING_QUEUE_H__
#define __UTILS_RING_QUEUE_H__

#include <stdbool.h>
#include "core/num_defs.h"

typedef struct ring_queue 
{
    usize_ptr head;
    usize_ptr count;

    usize_ptr capacity;
    void** arr; // array of void*

} ring_queue_t;

void ring_queue_init(ring_queue_t* queue);
void ring_queue_init_capacity(ring_queue_t* queue, usize_ptr init_capacity);

bool  ring_queue_push(ring_queue_t* queue, void* item);
void* ring_queue_pop(ring_queue_t* queue);

bool ring_queue_is_empty(ring_queue_t* queue);

#endif // __UTILS_RING_QUEUE_H__