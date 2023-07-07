/*
 * kernel/port.h
 * © suhas pai
 */

#pragma once
#include <stdint.h>

uint8_t port_in8(uint16_t port);
uint16_t port_in16(uint16_t port);

void port_out8(uint16_t port, uint8_t value);
void port_out16(uint16_t port, uint16_t value);
