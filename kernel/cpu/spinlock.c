/*
 * kernel/cpu/spinlock.c
 * © suhas pai
 */

#include <stdatomic.h>

#include "asm/irqs.h"
#include "asm/pause.h"

#include "spinlock.h"

void spinlock_init(struct spinlock *const lock) {
    lock->flag = 0;
}

void spin_acquire(struct spinlock *const lock) {
    int expected = 0;
    while (!atomic_compare_exchange_weak(&lock->flag, &expected, 1)) {
        cpu_pause();
    }
}

void spin_release(struct spinlock *const lock) {
    lock->flag = 0;
}

int spin_acquire_with_irq(struct spinlock *const lock) {
    const bool irqs_enabled = are_interrupts_enabled();

    disable_interrupts();
    spin_acquire(lock);

    return irqs_enabled;
}

void spin_release_with_irq(struct spinlock *const lock, const int flag) {
    spin_release(lock);
    if (flag != 0) {
        enable_interrupts();
    }
}