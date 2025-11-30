#include "core/atomic_defs.h"

typedef struct spinlock
{
    atom_bool locked;
    uptr irq_data;
} spinlock_t;

void spinlock_initlock(spinlock_t* lock, bool start_locked);

void spinlock_lock(spinlock_t* lock);

bool spinlock_try_lock(spinlock_t* lock);

bool spinlock_is_locked(spinlock_t* lock);

void spinlock_unlock(spinlock_t* lock);
