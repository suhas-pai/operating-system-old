/*
 * kernel/arch/x86_64/apic/init.c
 * © suhas pai
 */

#include "acpi/api.h"

#include "apic/lapic.h"
#include "apic/structs.h"

#include "asm/irqs.h"
#include "asm/msr.h"

#include "cpu/isr.h"
#include "dev/printk.h"
#include "sys/pic.h"

#include "cpu.h"
#include "ioapic.h"

void lapic_timer_irq_callback() {
    get_cpu_info_mut()->timer_ticks++;
    if (get_cpu_info()->timer_ticks % 1000 == 0) {
        printk(LOGLEVEL_INFO,
               "Timer: %" PRIu64 "\n",
               get_cpu_info_mut()->timer_ticks);
    }
}

enum {
    IA32_MSR_APIC_BASE_IS_BSP = 1 << 8,
    IA32_MSR_APIC_BASE_X2APIC = 1 << 10,
    IA32_MSR_APIC_BASE_ENABLE = 1 << 11
};

extern struct mmio_region *lapic_mmio_region;

void apic_init(const uint64_t local_apic_base) {
    lapic_mmio_region =
        vmap_mmio(range_create(local_apic_base, PAGE_SIZE),
                  PROT_READ | PROT_WRITE,
                  /*flags=*/0);

    assert_msg(lapic_mmio_region != NULL,
               "apic: failed to mmio-map local-apic registers");

    lapic_regs = lapic_mmio_region->base;

    pic_disable();
    printk(LOGLEVEL_INFO, "apic: local apic id: %x\n", lapic_regs->id);
    printk(LOGLEVEL_INFO,
           "apic: local apic version reg: %" PRIx32 ", version: %" PRIu32 "\n",
           lapic_regs->version,
           (lapic_regs->version & F_LAPIC_VERSION_REG_VERION_MASK));

    uint64_t apic_msr = read_msr(IA32_MSR_APIC_BASE);

    printk(LOGLEVEL_INFO, "apic: msr: 0x%" PRIx64 "\n", apic_msr);
    assert_msg((apic_msr & IA32_MSR_APIC_BASE_IS_BSP) != 0,
               "apic: cpu is not bsp");

    /* Use the x2apic if available */
    if (get_cpu_capabilities()->supports_x2apic) {
        apic_msr |= IA32_MSR_APIC_BASE_X2APIC;
    } else {
        printk(LOGLEVEL_INFO,
               "apic: x2apic not supported. reverting to xapic instead\n");
    }

    write_msr(IA32_MSR_APIC_BASE, apic_msr | IA32_MSR_APIC_BASE_ENABLE);

    lapic_init();
    isr_assign_irq_to_self_cpu(IRQ_TIMER,
                               isr_get_timer_vector(),
                               lapic_timer_irq_callback,
                               /*masked=*/false);
}