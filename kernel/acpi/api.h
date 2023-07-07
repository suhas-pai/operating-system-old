/*
 * kernel/acpi/init.h
 * © suhas pai
 */

#pragma once
#include "acpi/structs.h"

struct acpi_info {
    const struct acpi_madt *madt;
    const struct acpi_fadt *fadt;

    const struct acpi_rsdp *rsdp;
    const struct acpi_rsdt *rsdt;
};

void acpi_init();
struct acpi_sdt *acpi_lookup_sdt(const char signature[static 4]);

const struct acpi_info *get_acpi_info();
struct acpi_info *get_acpi_info_mut();