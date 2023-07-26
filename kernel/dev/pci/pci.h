/*
 * kernel/dev/pci/pci.h
 * © suhas pai
 */

#pragma once

#include <stdint.h>

#include "dev/pci/structs.h"
#include "lib/adt/range.h"
#include "lib/list.h"
#include "mm/mmio.h"

struct pci_config_space {
    uint16_t group_segment; /* PCIe only. 0 for PCI Local bus */

    uint8_t bus;
    uint8_t device_slot;
    uint8_t function;
};

enum pci_device_msi_support {
    PCI_DEVICE_MSI_SUPPORT_NONE,
    PCI_DEVICE_MSI_SUPPORT_MSI,
    PCI_DEVICE_MSI_SUPPORT_MSIX,
};

struct pci_device_bar_info {
    union {
        struct range port_range;
        struct {
            struct mmio_region *mmio;
            uint32_t index_in_mmio;
        };
    };

    bool is_present : 1;
    bool is_mmio : 1;
    bool is_prefetchable : 1;
    bool is_64_bit : 1;
};

uint8_t
pci_device_bar_read8(struct pci_device_bar_info *const bar,
                     const uint32_t offset);

uint16_t
pci_device_bar_read16(struct pci_device_bar_info *const bar,
                     const uint32_t offset);

uint32_t
pci_device_bar_read32(struct pci_device_bar_info *const bar,
                     const uint32_t offset);

uint64_t
pci_device_bar_read64(struct pci_device_bar_info *const bar,
                     const uint32_t offset);


struct pci_device_info {
    struct list list;
    struct pci_config_space config_space;
    struct pci_group *group;

    volatile const struct pci_spec_device_info *pcie_info;

    uint16_t id;
    uint16_t vendor_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;

    /* prog_if = "Programming Interface" */
    uint8_t prog_if;
    uint8_t header_kind;

    uint8_t subclass;
    uint8_t class;
    uint8_t irq_pin;

    bool multifunction : 1;
    uint8_t max_bar_count : 3;

    enum pci_device_msi_support msi_support : 2;

    union {
        uint64_t msi_offset;

        volatile struct pci_spec_cap_msi *pcie_msi;
        volatile struct pci_spec_cap_msix *pcie_msix;
    };

    //struct bitmap msix_table;
    struct pci_device_bar_info *bar_list;
};

#define PCI_DEVICE_INFO_FMT                                                    \
    "%" PRIx8 ":%" PRIx8 ":%" PRIx8 ":%" PRIx16 ":%" PRIx16 " (%" PRIx8 ":%"   \
    PRIx8 ")"

#define PCI_DEVICE_INFO_FMT_ARGS(device)                                       \
    (device)->config_space.bus,                                                \
    (device)->config_space.device_slot,                                        \
    (device)->config_space.function,                                           \
    (device)->vendor_id,                                                       \
    (device)->id,                                                              \
    (device)->class,                                                           \
    (device)->subclass

struct pci_group {
    struct list list;
    struct list device_list;

    // Only valid for PCIe
    struct range bus_range;
    struct mmio_region *mmio;

    uint16_t segment;

    uint32_t
    (*read)(const struct pci_device_info *s,
            uint32_t offset,
            uint8_t access_size);

    bool
    (*write)(const struct pci_device_info *space,
             uint32_t offset,
             uint32_t value,
             uint8_t access_size);
};

struct pci_group *
pci_group_create_pcie(struct range bus_range,
                      struct mmio_region *const mmio,
                      uint16_t segment);

void pci_init();