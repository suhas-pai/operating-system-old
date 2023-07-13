/*
 * kernel/arch/aarch64/asm/ttbr.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

static inline uint64_t read_ttbr0_base_address() {
    uint64_t val = 0;
    asm volatile("mrs %0, ttbr0_el1" : "=r" (val));

    return (val & 0xffffffffffc0) | ((val & 0x3c) << 46);
}

static inline uint64_t read_ttbr1_base_address() {
    uint64_t val = 0;
    asm volatile("mrs %0, ttbr1_el1" : "=r" (val));

    return (val & 0xffffffffffc0) | ((val & 0x3c) << 46);
}