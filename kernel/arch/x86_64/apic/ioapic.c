/*
 * kernel/arch/x86_64/apic/ioapic.c
 * © suhas pai
 */

#include "acpi/api.h"
#include "lib/inttypes.h"

#include "ioapic.h"

uint64_t
create_ioapic_redirect_request(
    const uint8_t vector,
    const enum ioapic_redirect_req_delivery_mode delivery_mode,
    const enum ioapic_redirect_req_dest_mode dest_mode,
    const uint32_t flags,
    const bool masked,
    const uint8_t lapic_id)
{
    const bool is_active_low = (flags & __ACPI_MADT_ENTRY_ISO_ACTIVE_LOW) != 0;
    const bool is_level_triggered =
        (flags & __ACPI_MADT_ENTRY_ISO_LEVEL_TRIGGER) != 0;

    const uint64_t result =
        vector |
        delivery_mode << 8 |
        dest_mode << 11 |
        (uint32_t)is_active_low << 13 |
        (uint32_t)is_level_triggered << 15 |
        ((uint32_t)masked << 16) |
        (uint64_t)lapic_id << 56;

    return result;
}

static struct ioapic_info *ioapic_info_for_gsi(const uint32_t gsi) {
    array_foreach (&get_acpi_info()->ioapic_list, struct ioapic_info, item) {
        const uint32_t gsi_base = item->gsi_base;
        if (gsi_base <= gsi && gsi_base + item->max_redirect_count > gsi) {
            return item;
        }
    }

    return NULL;
}

static void
redirect_irq(const uint8_t lapic_id,
             const uint8_t irq,
             const uint8_t vector,
             const uint16_t flags,
             const bool masked)
{
    const struct ioapic_info *const ioapic = ioapic_info_for_gsi(/*gsi=*/irq);
    assert_msg(ioapic != NULL,
               "mm: failed to find i/o apic for requested IRQ: %" PRIu8,
               irq);

    const uint8_t redirect_table_index = irq - ioapic->gsi_base;
    const uint64_t req_value =
        create_ioapic_redirect_request(vector,
                                       IOAPIC_REDIRECT_REQ_DELIVERY_MODE_FIXED,
                                       IOAPIC_REDIRECT_REQ_DEST_MODE_PHYSICAL,
                                       flags,
                                       masked,
                                       lapic_id);

    const uint32_t reg =
        ioapic_redirect_table_get_reg_for_n(redirect_table_index);

    /*
     * For selector=reg, write the lower-32 bits, for selector=reg + 1, write
     * the upper-32 bits.
     */

    ioapic_write(ioapic->regs_mmio->base, reg, req_value);
    ioapic_write(ioapic->regs_mmio->base, reg + 1, req_value >> 32);
}

/******* PUBLIC APIs *******/

uint32_t
ioapic_read(volatile struct ioapic_registers *const ioapic_regs,
            const enum ioapic_reg reg)
{
    ioapic_regs->selector = (uint32_t)reg;
    return ioapic_regs->data;
}

void
ioapic_write(volatile struct ioapic_registers *const ioapic_regs,
             const enum ioapic_reg reg,
             const uint32_t value)
{
    ioapic_regs->selector = (uint32_t)reg;
    ioapic_regs->data = value;
}

void
ioapic_redirect_irq(const uint8_t lapic_id,
                    const uint8_t irq,
                    const uint8_t vector,
                    const bool masked)
{
    /*
     * First check if the IOAPIC already directs this IRQ to the requested
     * vector. If so, we don't need to do anything.
     */

    array_foreach (&get_acpi_info()->iso_list, struct apic_iso_info, item) {
        if (item->irq_src != irq) {
            continue;
        }

        /*
         * Don't fill up the redirection-table if the iso already directs
         * the irq to the requested irq.
         */

        const uint8_t gsi = item->gsi;
        if (gsi == vector) {
            return;
        }

        /*
         * Create a redirection-request for the requested vector with an iso
         * (Interrupt Source Override) entry.
         */

        redirect_irq(lapic_id, gsi, vector, item->flags, masked);
        return;
    }

    /*
     * With no ISO found, we simply create a redirection-request for the irq we
     * were given.
     */

    redirect_irq(lapic_id, irq, vector, /*flags=*/0, masked);
}