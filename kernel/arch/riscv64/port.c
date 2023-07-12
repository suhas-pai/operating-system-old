/*
 * kernel/arch/riscv64/port.c
 * © suhas pai
 */

#include "port.h"

static inline void io_br_fence() {
    asm volatile("fence i,r" ::: "memory");
}

uint8_t port_in8(const port_t port) {
    uint8_t result = 0;
    asm volatile("lb %0, 0(%1)" : "=r"(result) : "r"(port));
    io_br_fence();

    return result;
}

uint16_t port_in16(const port_t port) {
    uint16_t result = 0;
    asm volatile("lh %0, 0(%1)" : "=r"(result) : "r"(port));
    io_br_fence();

    return result;
}

uint32_t port_in32(const port_t port) {
    uint32_t result = 0;
    asm volatile("lw %0, 0(%1)" : "=r"(result) : "r"(port));
    io_br_fence();

    return result;
}

uint64_t port_in64(const port_t port) {
    uint64_t result = 0;
    asm volatile("ld %0, 0(%1)" : "=r"(result) : "r"(port));
    io_br_fence();

    return result;
}

static inline void io_bw_fence() {
    asm volatile ("fence w,o" : : : "memory");
}

void port_out8(const port_t port, const uint8_t value) {
    io_bw_fence();
    asm volatile("sb %0, 0(%1)" :: "r"(value), "r"(port));
}

void port_out16(const port_t port, const uint16_t value) {
    io_bw_fence();
    asm volatile("sh %0, 0(%1)" :: "r"(value), "r"(port));
}

void port_out32(const port_t port, const uint32_t value) {
    io_bw_fence();
    asm volatile("sw %0, 0(%1)" :: "r"(value), "r"(port));
}

void port_out64(const port_t port, const uint64_t value) {
    io_bw_fence();
    asm volatile("sd %0, 0(%1)" :: "r"(value), "r"(port));
}