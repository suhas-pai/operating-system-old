/*
 * kernel/arch/x86_64/asm/irqs.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include "rflags.h"

enum irq_number {
    IRQ_TIMER    = 0,
    IRQ_KEYBOARD = 1,
};

static inline bool are_interrupts_enabled() {
    return (x86_64_read_rflags() & RFLAGS_INTERRUPTS_ENABLED) != 0;
}

static inline void disable_all_interrupts() { asm volatile("cli"); }
static inline void enable_all_interrupts() { asm volatile("sti"); }

static inline bool disable_all_int_if_not() {
    const bool result = are_interrupts_enabled();
    disable_all_interrupts();

    return result;
}

static inline void enable_all_int_if_flag(const bool flag) {
    if (flag) {
        enable_all_interrupts();
    }
}


