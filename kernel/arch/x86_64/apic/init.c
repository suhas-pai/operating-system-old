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

#include "cpu.h"
#include "pic.h"

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

void apic_init() {
    pic_disable();
    volatile struct lapic_registers *const info =
        get_acpi_info()->lapic_regs->base;

    printk(LOGLEVEL_INFO, "apic: local apic id: %x\n", info->id);
    printk(LOGLEVEL_INFO,
           "apic: local apic version reg: %" PRIx32 ", version: %" PRIu32 "\n",
           info->version,
           (info->version & F_LAPIC_VERSION_REG_VERION_MASK));

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
    const uint16_t ioapic_count =
        array_item_count(get_acpi_info()->ioapic_list);

    printk(LOGLEVEL_INFO,
           "apic: found %" PRIu16 " i/o apic(s)\n",
           ioapic_count);

    array_foreach (&get_acpi_info()->ioapic_list, struct ioapic_info, item) {
        printk(LOGLEVEL_INFO, "apic: ioapic id: %" PRIu8 "\n", item->id);
        printk(LOGLEVEL_INFO,
               "\tioapic version: %" PRIu8 "\n",
               item->version);
        printk(LOGLEVEL_INFO,
               "\tioapic max redirect count: %" PRIu8 "\n",
               item->max_redirect_count);
    }

    lapic_init();
    isr_assign_irq_to_self_cpu(IRQ_TIMER,
                               isr_get_timer_vector(),
                               lapic_timer_irq_callback,
                               /*masked=*/false);
}