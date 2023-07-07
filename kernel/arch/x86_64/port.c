
/*
 * kernel/arch/x86_64/port.c
 * © suhas pai
 */

#include "port.h"

uint8_t port_in8(const uint16_t port) {
    uint8_t result = 0;
    asm volatile (
        "in %1, %0\n\t"
        : "=a" (result)
        : "Nd" (port)
        : "memory");

    return result;
}

uint16_t port_in16(const uint16_t port) {
    uint16_t result = 0;
    asm volatile ("in %1, %0" : "=a" (result) : "Nd" (port));

    return result;
}

void port_out8(const uint16_t port, const uint8_t value) {
    asm volatile ("out %1, %0" : : "Nd" (port), "a" (value));
}

void port_out16(const uint16_t port, const uint16_t value) {
    asm volatile ("out %1, %0" : : "Nd" (port), "a" (value));
}