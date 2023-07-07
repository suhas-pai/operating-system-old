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

#define SPINLOCK_INIT(name) ((struct spinlock) {.flag = 0 })
void spinlock_init(struct spinlock *lock);

void spin_acquire(struct spinlock *lock);
void spin_release(struct spinlock *lock);

int spin_acquire_with_irq(struct spinlock *lock);
void spin_release_with_irq(struct spinlock *lock, int flag);