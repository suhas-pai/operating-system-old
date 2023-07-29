/*
 * kernel/dev/dtb/driver.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

struct dtb_driver {
    void (*init)(const void *dtb, int nodeoff);

    const char **compat_list;
    const uint32_t compat_count;
};