/*
 * kernel/arch/riscv64/asm/irqs.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

static inline void disable_interrupts(void) {
    asm volatile ("csrci sstatus, 0x2" ::: "memory");
}

static inline void enable_interrupts(void) {
    asm volatile ("csrsi sstatus, 0x2" ::: "memory");
}

static inline bool are_interrupts_enabled() {
    uint64_t info = 0;
    asm volatile("csrr %0, sstatus" : "=r" (info));

    return !(info & 0x2);
}