/*
 * kernel/arch/x86_64/asm/rflags.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>
#include "lib/macros.h"

enum rflags {
    RFLAGS_CARRY = 1 << 0,
    RFLAGS_PARITY = 1 << 2,

    /* Auxilary carry */
    RFLAGS_ADJUST = 1 << 4,
    RFLAGS_ZERO = 1 << 6,
    RFLAGS_SIGN = 1 << 7,

    /* Single-Step Interrupts */
    RFLAGS_TRAP = 1 << 8,
    RFLAGS_INTERRUPTS_ENABLED = 1 << 9,

    /* String-processing direction */
    RFLAGS_DIRECTION = 1 << 10,
    RFLAGS_OVERFLOW = 1 << 11,

    RFLAGS_IO_PRIVILEGE_LEVEL = (1 << 13) | (1 << 12),
    RFLAGS_RESUME = 1 << 16,
    RFLAGS_VIRTUAL_8086_MODE = 1 << 17,
    RFLAGS_ALIGNMENT_CHECK = 1 << 18,

    RFLAGS_VIRTUAL_INTERRUPT_FLAG = 1 << 19,
    RFLAGS_VIRTUAL_INTERRUPT_PENDING = 1 << 20,

    /* cpuid is supported if this bit can be modified. */
    RFLAGS_CPUID_BIT = 1 << 21
};

static inline uint64_t read_rflags() {
    uint64_t rflags = 0;
    asm volatile("pushfq;"
                 "popq %0" : "=r" (rflags) : : "memory");

    return rflags;
}

static inline void write_rflags(const uint64_t rflags) {
    asm volatile("push %0;"
                 "popfq" :: "r" (rflags) : "memory");
}