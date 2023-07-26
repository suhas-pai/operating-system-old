/*
 * lib/refcount.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "refcount.h"

void refcount_init(struct refcount *const ref) {
    ref->count = 1;
}

void refcount_init_max(struct refcount *const ref) {
    ref->count = REFCOUNT_MAX;
}

void refcount_increment(struct refcount *const ref, const int32_t amount) {
    const int32_t old =
        atomic_fetch_add_explicit(&ref->count, amount, memory_order_relaxed);

    if (old == 0) {
        panic("Either refcount not initialized or UAF");
    }

    if (old < 0 || old < amount) {
        panic("UAF in refcount_increment()");
    }

    if (old == REFCOUNT_MAX) {
        panic("refcount_increment() called on maxed refcount");
    }
}

bool refcount_decrement(struct refcount *const ref, const int32_t amount) {
    const int32_t old =
        atomic_fetch_sub_explicit(&ref->count, amount, memory_order_relaxed);

    if (old < 0 || old < amount) {
        panic("UAF in refcount_decrement()");
    }

    if (old == REFCOUNT_MAX) {
        panic("refcount_decrement() called on maxed refcount");
    }

    return old == 1;
}

void ref_up(struct refcount *const ref) {
    refcount_increment(ref, /*amount=*/1);
}

bool ref_down(struct refcount *const ref) {
    return refcount_decrement(ref, /*amount=*/1);
}