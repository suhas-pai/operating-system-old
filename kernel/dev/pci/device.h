/*
 * kernel/dev/pci/device.h
 * © suhas pai
 */

#pragma once

#if defined(__x86_64__)
    #include "arch/x86_64/cpu.h"
#endif /* defined(__x86_64__) */

#include "lib/adt/bitmap.h"

#include "cpu/isr.h"
#include "mm/mmio.h"

struct pci_config_space {
    uint16_t domain_segment; /* PCIe only. 0 for PCI Local bus */

    uint8_t bus;
    uint8_t device_slot;
    uint8_t function;
};

uint32_t pci_config_space_get_index(struct pci_config_space config_space);

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

struct pci_device_info;
struct pci_domain {
    struct list list;
    struct list device_list;

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

struct pci_device_info {
    struct list list_in_domain;
    struct list list_in_devices;

    struct pci_domain *domain;
    struct pci_config_space config_space;

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

    bool supports_pcie : 1;
    bool multifunction : 1;
    uint8_t max_bar_count : 3;

    enum pci_device_msi_support msi_support : 2;

    union {
        uint64_t pcie_msi_offset;
        struct {
            uint64_t pcie_msix_offset;
            struct bitmap msix_table;
        };
    };

    struct pci_device_bar_info *bar_list;
};

uint32_t pci_device_get_index(const struct pci_device_info *const device);

#define pci_read(device, type, field) \
    (device)->domain->read((device), \
                           offsetof(type, field), \
                           sizeof_field(type, field))

#define pci_write(device, type, field, value) \
    (device)->domain->write((device),\
                            offsetof(type, field),\
                            sizeof_field(type, field),\
                            value)

#define pci_read_with_offset(device, offset, type, field) \
    (device)->domain->read((device), \
                           (offset) + offsetof(type, field), \
                           sizeof_field(type, field))

#define pci_write_with_offset(device, offset, type, field, value) \
    (device)->domain->write((device),\
                            (offset) + offsetof(type, field), \
                            sizeof_field(type, field),\
                            value)

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

#if defined(__x86_64__)
    bool
    pci_device_bind_msi_to_vector(struct pci_device_info *device,
                                  const struct cpu_info *cpu,
                                  isr_vector_t vector,
                                  bool masked);
#endif

void pci_device_enable_bus_mastering(struct pci_device_info *device);