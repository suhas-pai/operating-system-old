/*
 * kernel/acpi/mcfg.c
 * © suhas pai
 */

#include "dev/pci/pci.h"
#include "dev/printk.h"

#include "lib/align.h"

#include "mm/kmalloc.h"
#include "mm/vma.h"

#include "mcfg.h"

void mcfg_init(const struct acpi_mcfg *const mcfg) {
    const uint32_t length = mcfg->sdt.length - sizeof(*mcfg);
    const uint32_t entry_count = length / sizeof(struct acpi_mcfg_entry);

    for (uint32_t index = 0; index != entry_count; index++) {
        const struct acpi_mcfg_entry *const iter = mcfg->entries + index;
        printk(LOGLEVEL_INFO,
               "mcfg: pci-domain #%d: mmio at %p, first bus=%" PRIu32 ", end "
               "bus=%" PRIu32 ", segment: %" PRIu32 "\n",
               index + 1,
               (void *)iter->base_addr,
               iter->bus_start_num,
               iter->bus_end_num,
               iter->segment_num);

        const struct range bus_range =
            range_create(iter->bus_start_num,
                         iter->bus_end_num - iter->bus_start_num);

        pci_group_create_pcie(bus_range, iter->base_addr, iter->segment_num);
    }
}