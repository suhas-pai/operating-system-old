/*
 * lib/refcount.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "refcount.h"

void refcount_init(struct refcount *const ref) {
    ref->count = 1;
}

void refcount_increment(struct refcount *const ref, const int32_t amount) {
    const int32_t old =
        atomic_fetch_add_explicit(&ref->count, 1, memory_order_relaxed);

    if (old == 0) {
        panic("Either refcount not initialized or UAF");
    }

    if (old < 0 || old < amount) {
        panic("UAF in refcount_increment()");
    }
}

void refcount_decrement(struct refcount *const ref, const int32_t amount) {
    const int32_t old =
        atomic_fetch_sub_explicit(&ref->count, 1, memory_order_relaxed);

    if (old < 0 || old < amount) {
        panic("UAF in refcount_decrement()");
    }
}