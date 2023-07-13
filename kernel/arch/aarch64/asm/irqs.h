/*
 * kernel/arch/aarch64/asm/irqs.h
 * © suhas pai
 */

#pragma once
#include <stdbool.h>

static inline void disable_all_interrupts(void) {
    asm volatile("msr daifset, #15");
}

static inline void enable_all_interrupts(void) {
    asm volatile("msr daifclr, #15");
}

static inline bool are_interrupts_enabled() {
    return true;
}