#include "services/threads/locks/spinlock.h"
#include "kernel/core/cpu.h"
#include "kernel/interrupts/irq.h"
#include <stdatomic.h>

void spinlock_initlock(spinlock_t* lock, bool start_locked)
{
    lock->locked = false; // not atom on init
    
    if (start_locked)
    {
        spinlock_lock(lock);
    }
}

void spinlock_lock(spinlock_t* lock)
{
    uptr irq_state = irq_save();

    // acquire must as a "compiler hint"
    // return old, while (it was true, cpu_relax())
    while (atomic_exchange_explicit(
        &lock->locked,
        true,
        memory_order_acquire))
    {
        cpu_relax();
    }

    lock->irq_data = irq_state;
}

bool spinlock_try_lock(spinlock_t* lock)
{
    uptr irq_state = irq_save();

    bool was_locked = atomic_exchange_explicit(
        &lock->locked,
        true,
        memory_order_acquire
    );

    if (was_locked == false)
    {
        lock->irq_data = irq_state;

        return true;
    }

    irq_restore(irq_state);

    return false;
}

bool spinlock_is_locked(spinlock_t* lock)
{
    return atomic_load(&lock->locked);
}

void spinlock_unlock(spinlock_t *lock)
{
    atomic_store_explicit(
        &lock->locked,
        false,
        memory_order_release
    );

    irq_restore(lock->irq_data);

    lock->irq_data = 0;
}