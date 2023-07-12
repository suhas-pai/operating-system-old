/*
 * kernel/arch/x86_64/apic/ioapic.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lib/macros.h"

struct ioapic_registers {
    _Alignas(16) volatile uint32_t selector; /* IOREGSEL */
    _Alignas(16) volatile uint32_t data;     /* IOWIN */
} __packed;

enum ioapic_redirect_req_delivery_mode {
    IOAPIC_REDIRECT_REQ_DELIVERY_MODE_FIXED,
    IOAPIC_REDIRECT_REQ_DELIVERY_MODE_LOWEST,

    /* SMI = "System Management Interrupt" */
    IOAPIC_REDIRECT_REQ_DELIVERY_MODE_SMI,

    /* NMI = "Non-maskable Interrupt" */
    IOAPIC_REDIRECT_REQ_DELIVERY_MODE_NMI = 0b100,
    IOAPIC_REDIRECT_REQ_DELIVERY_MODE_INIT,

    /* ExtINT = "External Interrupt" */
    IOAPIC_REDIRECT_REQ_DELIVERY_MODE_EXTINT = 0b111,
};

enum ioapic_redirect_req_dest_mode {
    IOAPIC_REDIRECT_REQ_DEST_MODE_PHYSICAL,
    IOAPIC_REDIRECT_REQ_DEST_MODE_LOGICAL,
};

enum ioapic_redirect_req_pin_polarity {
    IOAPIC_REDIRECT_REQ_PIN_POLARITY_ACTIVE_HIGH,
    IOAPIC_REDIRECT_REQ_PIN_POLARITY_ACTIVE_LOW
};

enum ioapic_redirect_req_trigger_mode {
    IOAPIC_REDIRECT_REQ_PIN_TRIGGER_MODE_EDGE,
    IOAPIC_REDIRECT_REQ_PIN_TRIGGER_MODE_LEVEL
};

enum ioapic_reg {
    IOAPIC_REG_ID,
    IOAPIC_REG_VERSION,
    IOAPIC_REG_ARBITRATION_ID,
    IOAPIC_REG_REDIRECTION_TABLE_BASE = 0x10,
};

uint32_t
ioapic_read(volatile struct ioapic_registers *ioapic_regs, enum ioapic_reg reg);

void
ioapic_write(volatile struct ioapic_registers *ioapic_regs,
             enum ioapic_reg reg,
             uint32_t value);

void
ioapic_redirect_irq(uint8_t lapic_id, uint8_t irq, uint8_t vector, bool masked);

static inline uint8_t ioapic_id_reg_get_id(const uint32_t version) {
    /* Bits [0:8] of the id register holds the id */
    return (version >> 24) & 0b1111;
}

static inline uint8_t ioapic_version_reg_get_version(const uint32_t version) {
    /* Bits [0:8] of the version register holds the version, minus */
    return (uint8_t)version;
}

static inline
uint32_t ioapic_version_reg_get_max_redirect_count(const uint32_t version) {
    /*
     * Bits [16:23] of the version register holds the max-redirect count, minus
     * one.
     */

    return (uint8_t)(version >> 16) + 1;
}

static inline uint32_t ioapic_redirect_table_get_reg_for_n(const uint8_t n) {
    return (uint32_t)(IOAPIC_REG_REDIRECTION_TABLE_BASE + (n * 2));
}