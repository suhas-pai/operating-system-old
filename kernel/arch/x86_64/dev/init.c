/*
 * kernel/arch/x86_64/dev/init.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "dev/ps2/driver.h"
#include "dev/time/hpet.h"

#include "acpi/api.h"

void arch_init_dev() {
    const struct acpi_fadt *const fadt = get_acpi_info()->fadt;
    if (fadt != NULL &&
        (fadt->iapc_boot_arch_flags & __ACPI_FADT_IAPC_BOOT_8042) != 0)
    {
        ps2_init();
    } else {
        printk(LOGLEVEL_WARN, "dev: ps2 keyboard/mouse are not supported\n");
    }

    const struct acpi_hpet *const hpet =
        (const struct acpi_hpet *)acpi_lookup_sdt("HPET");

    assert_msg(hpet != NULL, "dev: hpet not found. aborting init");
    hpet_init(hpet);
}