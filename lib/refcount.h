/*
 * lib/refcount.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct refcount {
    _Atomic int32_t count;
};

#define REFCOUNT_SATURATED (INT32_MAX / 2)
#define REFCOUNT_MAX INT32_MAX

void refcount_init(struct refcount *ref);

void refcount_increment(struct refcount *ref, int32_t amount);
void refcount_decrement(struct refcount *ref, int32_t amount);

