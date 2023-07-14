/*
 * kernel/arch/riscv64/port.c
 * © suhas pai
 */

#include "port.h"

uint8_t port_in8(const port_t port) {
    return *(volatile uint8_t *)port;
}

uint16_t port_in16(const port_t port) {
    return *(volatile uint16_t *)port;
}

uint32_t port_in32(const port_t port) {
    return *(volatile uint32_t *)port;
}

uint64_t port_in64(const port_t port) {
    return *(volatile uint64_t *)port;
}

void port_out8(const port_t port, const uint8_t value) {
    *(volatile uint8_t *)port = value;
}

void port_out16(const port_t port, const uint16_t value) {
    *(volatile uint16_t *)port = value;
}

void port_out32(const port_t port, const uint32_t value) {
    *(volatile uint32_t *)port = value;
}

void port_out64(const port_t port, const uint64_t value) {
    *(volatile uint64_t *)port = value;
}