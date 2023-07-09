/*
 * kernel/cpu/spin.h
 * © suhas pai
 */

#pragma once

#include <stdatomic.h>
#include <stdbool.h>

struct spinlock {
    _Atomic int flag;
};

void spinlock_init(struct spinlock *lock);

void spin_acquire(struct spinlock *lock);
void spin_release(struct spinlock *lock);

int spin_acquire_with_irq(struct spinlock *lock);
void spin_release_with_irq(struct spinlock *lock, int flag);