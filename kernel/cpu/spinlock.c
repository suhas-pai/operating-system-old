/*
 * kernel/cpu/spinlock.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "asm/irqs.h"
#include "asm/pause.h"

#include "spinlock.h"

__optimize(3) void spinlock_init(struct spinlock *const lock) {
    lock->front = 0;
    lock->back = 0;
}

__optimize(3) void spin_acquire(struct spinlock *const lock) {
    const uint64_t ticket = atomic_fetch_add(&lock->back, 1);
    while (true) {
        const uint64_t front = atomic_load(&lock->front);
        if (front == ticket) {
            return;
        }
    }
}

__optimize(3) void spin_release(struct spinlock *const lock) {
    atomic_fetch_add(&lock->front, 1);
}

__optimize(3) int spin_acquire_with_irq(struct spinlock *const lock) {
    const bool irqs_enabled = are_interrupts_enabled();

    disable_all_interrupts();
    spin_acquire(lock);

    return irqs_enabled;
}

__optimize(3)
void spin_release_with_irq(struct spinlock *const lock, const int flag) {
    spin_release(lock);
    if (flag != 0) {
        enable_all_interrupts();
    }
}