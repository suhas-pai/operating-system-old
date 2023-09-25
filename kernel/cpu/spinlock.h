/*
 * kernel/cpu/spinlock.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

struct spinlock {
    _Atomic uint64_t front;
    _Atomic uint64_t back;
};

#define SPINLOCK_INIT() ((struct spinlock){ .front = 0, .back = 0 })

void spinlock_init(struct spinlock *lock);

void spin_acquire(struct spinlock *lock);
void spin_release(struct spinlock *lock);

int spin_acquire_with_irq(struct spinlock *lock);
void spin_release_with_irq(struct spinlock *lock, int flag);