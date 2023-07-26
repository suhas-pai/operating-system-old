/*
 * kernel/dev/resource.h
 * © suhas pai
 */

#pragma once
#include "cpu/spinlock.h"

struct resource {
    struct spinlock lock;
};