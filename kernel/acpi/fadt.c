/*
 * kernel/acpi/fadt.c
 * © suhas pai
 */

#include "dev/printk.h"

#if defined(__x86_64__)
    #include "dev/ps2/driver.h"
#endif /* defined(__x86_64__) */

#include "fadt.h"

void fadt_init(const struct acpi_fadt *const fadt) {
    printk(LOGLEVEL_INFO,
           "fadt: Version %" PRIu8 ".%" PRIu8 "\n",
           fadt->sdt.rev,
           fadt->fadt_minor_version);

#if defined(__x86_64__)
    printk(LOGLEVEL_INFO,
           "fadt: Flags: %" PRIu32 "\n",
           fadt->iapc_boot_arch_flags);

    const bool fadt_flags_has_8042 =
        fadt->iapc_boot_arch_flags & F_ACPI_FADT_IAPC_BOOT_8042;

    if (fadt_flags_has_8042) {
        ps2_init();
    } else {
        printk(LOGLEVEL_WARN,
               "fadt: No i8042 found. No i8042 Keyboard/Mouse support\n");
    }
#elif defined(__aarch64__)
    printk(LOGLEVEL_INFO,
           "fadt: Flags: %" PRIu32 "\n",
           fadt->arm_boot_arch_flags);
#endif /* defined(__x86_64__) */
}