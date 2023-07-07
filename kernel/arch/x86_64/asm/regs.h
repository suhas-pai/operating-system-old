/*
 * kernel/arch/x86_64/asm/regs.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

static inline uint64_t read_cr3() {
    uint64_t cr3 = 0;
    asm volatile ("mov %%cr3, %%rax" : "=a"(cr3));

    return cr3;
}

static inline void write_cr3(const uint64_t cr3) {
    asm volatile ("mov %%rax, %%cr3" :: "a"(cr3) : "memory");
}
