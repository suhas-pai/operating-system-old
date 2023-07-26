/*
 * kernel/dev/pci/pci.c
 * © suhas pai
 */

#include "acpi/api.h"

#include "dev/pci/pci.h"
#include "dev/pci/structs.h"

#include "dev/printk.h"

#include "lib/align.h"
#include "lib/assert.h"
#include "lib/list.h"

#include "mm/kmalloc.h"
#include "mm/mm_types.h"
#include "mm/mmio.h"

#include "port.h"
#include "read.h"

static struct list group_list = LIST_INIT(group_list);
static uint64_t group_count = 0;

static uint32_t
pcie_read(const struct pci_device_info *const device,
          const uint32_t offset,
          const uint8_t access_size)
{
    struct pci_group *const group = device->group;
    const uint32_t index =
        (uint32_t)((device->config_space.bus - group->bus_range.front) << 20) |
        ((uint32_t)device->config_space.device_slot << 15) |
        ((uint32_t)device->config_space.function << 12);

    const struct range mmio_range = mmio_region_get_range(group->mmio);
    const struct range access_range = range_create(index + offset, access_size);

    if (!range_has_index_range(mmio_range, access_range)) {
        printk(LOGLEVEL_WARN,
                "pcie: attempting to read range " RANGE_FMT " which is "
                "outside mmio range of pcie group, " RANGE_FMT "\n",
                RANGE_FMT_ARGS(access_range),
                RANGE_FMT_ARGS(mmio_range));
        return (uint32_t)-1;
    }

    volatile void *const ptr = group->mmio->base + index + offset;
    switch (access_size) {
        case sizeof(uint8_t):
            return *(volatile uint8_t *)ptr;
        case sizeof(uint16_t):
            return *(volatile uint16_t *)ptr;
        case sizeof(uint32_t):
            return *(volatile uint32_t *)ptr;
    }

    verify_not_reached();
}

static bool
pcie_write(const struct pci_device_info *const device,
           const uint32_t offset,
           const uint32_t value,
           const uint8_t access_size)
{
    struct pci_group *const group = device->group;
    const uint64_t index =
        (uint64_t)((device->config_space.bus - group->bus_range.front) << 20) |
        ((uint64_t)device->config_space.device_slot << 15) |
        ((uint64_t)device->config_space.function << 12);

    const struct range mmio_range = mmio_region_get_range(group->mmio);
    const struct range access_range =
        range_create(index + offset, access_size);

    if (!range_has_index_range(mmio_range, access_range)) {
        printk(LOGLEVEL_WARN,
                "pcie: attempting to write to range " RANGE_FMT " which is "
                "outside mmio range of pcie group, " RANGE_FMT "\n",
                RANGE_FMT_ARGS(access_range),
                RANGE_FMT_ARGS(mmio_range));
        return (uint32_t)-1;
    }

    volatile void *const ptr = group->mmio->base + index + offset;
    switch (access_size) {
        case sizeof(uint8_t):
            *(volatile uint8_t *)ptr = (uint8_t)value;
            return true;
        case sizeof(uint16_t):
            *(volatile uint16_t *)ptr = (uint16_t)value;
            return true;
        case sizeof(uint32_t):
            *(volatile uint32_t *)ptr = value;
            return true;
    }

    verify_not_reached();
}

enum parse_bar_result {
    E_PARSE_BAR_OK,
    E_PARSE_BAR_IGNORE,

    E_PARSE_BAR_UNKNOWN_MEM_KIND,
    E_PARSE_BAR_NO_REG_FOR_UPPER32,
    E_PARSE_BAR_MMIO_MAP_FAIL
};

#define read_bar(index) \
    dev->group->read(   \
        dev,            \
        offsetof(struct pci_spec_general_device_info, bar_list) + \
            (index * sizeof(uint32_t)),                           \
        sizeof(uint32_t))

#define write_bar(index, value) \
    dev->group->write( \
        dev,           \
        offsetof(struct pci_spec_general_device_info, bar_list) + \
            (index * sizeof(uint32_t)), \
        value,                          \
        sizeof(uint32_t))

static enum parse_bar_result
pci_bar_parse_size(struct pci_device_info *const dev,
                   struct pci_device_bar_info *const info,
                   const uint64_t base_addr,
                   const uint32_t bar_0_index)
{
    if (base_addr == 0) {
        return E_PARSE_BAR_IGNORE;
    }

    /*
     * To get the size, we have to
     *   (1) read the BAR register and save the value.
     *   (2) write 0xFFFFFFFF to the BAR register, and read the new value.
     *   (3) Restore the original value
     *
     * Since we operate on two registers for 64-bit, we have to perform this
     * procedure on both registers for 64-bit.
     */

    const uint32_t bar_0_orig = read_bar(bar_0_index);
    write_bar(bar_0_index, 0xFFFFFFFF);

    const uint32_t bar_0_new = read_bar(bar_0_index);
    write_bar(bar_0_index, bar_0_orig);

    uint32_t bar_1_new = UINT32_MAX;
    if (info->is_64_bit) {
        const uint32_t bar_1_index = bar_0_index + 1;
        const uint32_t bar_1_orig = read_bar(bar_1_index);

        write_bar(bar_1_index, 0xFFFFFFFF);
        bar_1_new = read_bar(bar_1_index);
        write_bar(bar_1_index, bar_1_orig);
    }

    /*
     * To calculate the size, we re-calculate the base-address with the *new*
     * register-values, and invert the bits and add one.
     */

    uint64_t size =
        (uint64_t)bar_1_new << 32 |
        align_down(bar_0_new, info->is_mmio ? 16 : 4);

    size = ~size + 1;

    // We use port range to temporarily store the phys range.
    info->port_range = range_create(base_addr, size);
    info->is_present = true;

    return E_PARSE_BAR_OK;
}

/* index_in is incremented assuming index += 1 will be called by the caller */
static enum parse_bar_result
pci_parse_bar(struct pci_device_info *const dev,
              uint8_t *const index_in,
              const bool is_bridge,
              struct pci_device_bar_info *const bar)
{
    const uint8_t index = *index_in;
    const uint32_t bar_0 = read_bar(index);

    uint64_t base_addr = 0;
    if (bar_0 & __PCI_DEVBAR_IO) {
        base_addr = align_down(bar_0, /*boundary=*/4);
        return pci_bar_parse_size(dev, bar, base_addr, index);
    }

    bar->is_mmio = true;
    bar->is_prefetchable = (bar_0 & __PCI_DEVBAR_PREFETCHABLE) != 0;

    const enum pci_spec_dev_memspace_bar_kind memory_kind =
        (bar_0 & __PCI_DEVBAR_MEMKIND_MASK) >> 1;

    enum parse_bar_result result = E_PARSE_BAR_OK;
    if (memory_kind == __PCI_DEVBAR_MEMSPACE_32B) {
        base_addr = align_down(bar_0, /*boundary=*/16);
        result = pci_bar_parse_size(dev, bar, base_addr, index);

        goto mmio_map;
    }

    if (memory_kind != __PCI_DEVBAR_MEMSPACE_64B) {
        printk(LOGLEVEL_WARN, "pci: unknown memory kind %d\n", memory_kind);
        return E_PARSE_BAR_UNKNOWN_MEM_KIND;
    }

    /* Check if we even have another register left */
    const uint64_t last_index =
        is_bridge ?
            (PCI_BAR_COUNT_FOR_BRIDGE - 1) : (PCI_BAR_COUNT_FOR_GENERAL - 1);

    if (index == last_index) {
        printk(LOGLEVEL_INFO, "pci: 64-bit bar has no upper32 register\n");
        return E_PARSE_BAR_NO_REG_FOR_UPPER32;
    }

    const uint32_t bar_1 = read_bar(index + 1);

    bar->is_64_bit = true;
    base_addr = align_down(bar_0, /*boundary=*/16) | (uint64_t)bar_1 << 32;

    /* Increment once more since we read another register */
    *index_in += 1;
    result = pci_bar_parse_size(dev, bar, base_addr, index);

mmio_map:
    if (result != E_PARSE_BAR_OK) {
        return result;
    }

    // We use port_range to internally store the phys range.
    const struct range phys_range = bar->port_range;
    struct range aligned_range = {};

    if (!range_align_out(phys_range, PAGE_SIZE, &aligned_range)) {
        printk(LOGLEVEL_WARN,
               "pcie: failed to align map bar %" PRIu8 " for device\n",
               index);
        return E_PARSE_BAR_MMIO_MAP_FAIL;
    }

    struct mmio_region *const mmio =
        vmap_mmio(aligned_range, PROT_READ | PROT_WRITE);

    assert_msg(mmio != NULL,
               "pcie: failed to mmio map bar %" PRIu8 " at phys "
               "range: " RANGE_FMT,
               index,
               RANGE_FMT_ARGS(bar->port_range));

    const uint64_t index_in_map = bar->port_range.front - aligned_range.front;

    bar->mmio = mmio;
    bar->index_in_mmio = index_in_map;

    return E_PARSE_BAR_OK;
}

uint8_t
pci_device_bar_read8(struct pci_device_bar_info *const bar,
                     const uint32_t offset)
{
    return
        bar->is_mmio ?
            *reg_to_ptr(const uint8_t,
                        bar->mmio->base + bar->index_in_mmio,
                        offset) :
            port_in8((port_t)(bar->port_range.front + offset));
}

uint16_t
pci_device_bar_read16(struct pci_device_bar_info *const bar,
                      const uint32_t offset)
{
    return
        bar->is_mmio ?
            *reg_to_ptr(const uint16_t,
                        bar->mmio->base + bar->index_in_mmio,
                        offset) :
            port_in16((port_t)(bar->port_range.front + offset));
}

uint32_t
pci_device_bar_read32(struct pci_device_bar_info *const bar,
                      const uint32_t offset)
{
    return
        bar->is_mmio ?
            *reg_to_ptr(const uint32_t,
                        bar->mmio->base + bar->index_in_mmio,
                        offset) :
            port_in32((port_t)(bar->port_range.front + offset));
}

uint64_t
pci_device_bar_read64(struct pci_device_bar_info *const bar,
                      const uint32_t offset)
{
#if defined(__x86_64__)
    assert(bar->is_mmio);
    return
        *reg_to_ptr(const uint64_t,
                    bar->mmio->base + bar->index_in_mmio,
                    offset);
#else
    return
        (bar->is_mmio) ?
            *reg_to_ptr(const uint64_t,
                        bar->mmio->base + bar->index_in_mmio,
                        offset) :
            port_in64((port_t)(bar->port_range.front + offset));
#endif /* defined(__x86_64__) */
}

static void pci_parse_capabilities(struct pci_device_info *const dev) {
    uint8_t cap_offset =
        dev->group->read(dev,
                         offsetof(struct pci_spec_general_device_info,
                                  capabilities_offset),
                         sizeof_field(struct pci_spec_general_device_info,
                                      capabilities_offset));

    if (cap_offset == 0 || cap_offset == (uint8_t)-1) {
        printk(LOGLEVEL_INFO, "\t\thas no capabilities\n");
        return;
    }

    if (index_in_bounds(cap_offset,
                        sizeof(struct pci_spec_general_device_info)))
    {
        printk(LOGLEVEL_INFO,
               "pcie: invalid device. pci capability offset points to within "
               "structure: 0x%" PRIx8 "\n",
               cap_offset);
        return;
    }

    for (uint64_t i = 0; cap_offset != 0 && cap_offset != 0xff; i++) {
        if (i == 512) {
            printk(LOGLEVEL_INFO,
                   "pcie: too many capabilities for "
                   "device " PCI_DEVICE_INFO_FMT "\n",
                   PCI_DEVICE_INFO_FMT_ARGS(dev));
            return;
        }

    #define read_cap_field(field) \
        dev->group->read(dev,         \
                         cap_offset + \
                            offsetof(struct pci_spec_capability, field), \
                         sizeof_field(struct pci_spec_capability, field))

        volatile struct pci_spec_capability *const capability =
            reg_to_ptr(struct pci_spec_capability, dev->pcie_msi, cap_offset);

        const uint8_t id = read_cap_field(id);
        switch (id) {
            case PCI_SPEC_CAP_ID_MSI:
                if (dev->msi_support == PCI_DEVICE_MSI_SUPPORT_MSIX) {
                    break;
                }

                dev->msi_support = PCI_DEVICE_MSI_SUPPORT_MSI;
                dev->pcie_msi = (volatile struct pci_spec_cap_msi *)capability;

                break;
            case PCI_SPEC_CAP_ID_MSI_X:
                dev->msi_support = PCI_DEVICE_MSI_SUPPORT_MSIX;
                dev->pcie_msix =
                    (volatile struct pci_spec_cap_msix *)capability;

                break;
        }

        printk(LOGLEVEL_INFO,
               "\t\tfound capability: %" PRIu8 " at offset "
               "0x%" PRIx8 "\n",
               id,
               cap_offset);

        cap_offset = read_cap_field(offset_to_next);
    }
}

static volatile struct pci_spec_device_info *
pci_group_get_device_for_pos(const struct pci_group *const group,
                             const struct pci_config_space *const config_space)
{
    const uint32_t offset =
        ((uint32_t)(config_space->bus - group->bus_range.front) << 20) |
        ((uint32_t)config_space->device_slot << 15) |
        ((uint32_t)config_space->function << 12);

    if (!index_in_bounds(offset, group->mmio->size)) {
        printk(LOGLEVEL_WARN,
               "pcie: invalid request for device with offset: 0x%" PRIu32 ", "
               "total available size: %" PRIu32 "\n",
               offset,
               group->mmio->size);
        return NULL;
    }

    volatile struct pci_spec_device_info *const result =
        (volatile struct pci_spec_device_info *)(group->mmio->base + offset);

    if (result->vendor_id == 0xffff) {
        return NULL;
    }

    return result;
}

void
pci_parse_bus(struct pci_group *group,
              struct pci_config_space *config_space,
              uint8_t bus);

static void
parse_function(struct pci_group *const group,
               const struct pci_config_space *const config_space)
{

    struct pci_device_info info = {0};
    volatile struct pci_spec_device_info *device = NULL;
    uint8_t hdrkind = 0;

#define device_spec_get_field(field) \
    group->read(&info, \
                offsetof(struct pci_spec_device_info, field), \
                sizeof_field(struct pci_spec_device_info, field))

    if (group->mmio != NULL) {
        device = pci_group_get_device_for_pos(group, config_space);
        if (device == NULL) {
            return;
        }

        const bool is_multi_function =
            (device->header_kind & __PCI_DEVHDR_MULTFUNC) != 0;

        hdrkind = device->header_kind & (uint8_t)~__PCI_DEVHDR_MULTFUNC;
        volatile struct pci_spec_general_device_info *const general_device =
            (volatile struct pci_spec_general_device_info *)device;

        info = (struct pci_device_info){
            .group = group,
            .config_space = *config_space,
            .pcie_info = device,

            .id = device->device_id,
            .vendor_id = device->vendor_id,
            .command = device->command,
            .status = device->status,
            .revision_id = device->revision_id,

            .prog_if = device->prog_if,
            .header_kind = hdrkind,

            .class = device->class_code,
            .subclass = device->subclass,
            .irq_pin =
                hdrkind == PCI_SPEC_DEVHDR_KIND_GENERAL ?
                    general_device->interrupt_pin : 0,

            .multifunction = is_multi_function,
        };
    } else {
        const uint16_t vendor_id = device_spec_get_field(vendor_id);
        if (vendor_id == (uint16_t)-1) {
            return;
        }

        const uint8_t header_kind = device_spec_get_field(header_kind);
        const bool is_multi_function =
            (header_kind & __PCI_DEVHDR_MULTFUNC) != 0;

        hdrkind = header_kind & (uint8_t)~__PCI_DEVHDR_MULTFUNC;
        const uint8_t irq_pin =
            hdrkind == PCI_SPEC_DEVHDR_KIND_GENERAL ?
                group->read(&info,
                            offsetof(struct pci_spec_general_device_info,
                                     interrupt_pin),
                            sizeof_field(struct pci_spec_general_device_info,
                                         interrupt_pin))
            : 0;

        info = (struct pci_device_info){
            .group = group,
            .config_space = *config_space,

            .id = device_spec_get_field(device_id),
            .vendor_id = vendor_id,
            .command = device_spec_get_field(command),
            .status = device_spec_get_field(status),
            .revision_id = device_spec_get_field(revision_id),

            .prog_if = device_spec_get_field(prog_if),
            .header_kind = hdrkind,

            .class = device_spec_get_field(class_code),
            .subclass = device_spec_get_field(subclass),
            .irq_pin = irq_pin,

            .multifunction = is_multi_function,
        };
    }

    printk(LOGLEVEL_INFO,
           "\tdevice: " PCI_DEVICE_INFO_FMT "\n",
           PCI_DEVICE_INFO_FMT_ARGS(&info));

    const bool class_is_pci_bridge = info.class == 0x4 && info.subclass == 0x6;
    const bool hdrkind_is_pci_bridge =
        hdrkind == PCI_SPEC_DEVHDR_KIND_PCI_BRIDGE;

    if (hdrkind_is_pci_bridge != class_is_pci_bridge) {
        printk(LOGLEVEL_WARN,
            "pcie: invalid device, header-type and class/subclass "
            "mismatch\n");
        return;
    }

    switch (hdrkind) {
        case PCI_SPEC_DEVHDR_KIND_GENERAL: {
            const uint64_t bar_list_size =
                sizeof(struct pci_device_bar_info) * PCI_BAR_COUNT_FOR_GENERAL;

            info.bar_list = kmalloc(bar_list_size);
            if (info.bar_list == NULL) {
                printk(LOGLEVEL_WARN,
                       "pcie: failed to allocate memory for bar list\n");
                return;
            }

            info.max_bar_count = PCI_BAR_COUNT_FOR_GENERAL;
            pci_parse_capabilities(&info);

            for (uint8_t index = 0;
                 index != PCI_BAR_COUNT_FOR_GENERAL;
                 index++)
            {
                struct pci_device_bar_info *const bar = info.bar_list + index;
                const enum parse_bar_result result =
                    pci_parse_bar(&info, &index, /*is_bridge=*/false, bar);

                if (result == E_PARSE_BAR_IGNORE) {
                    printk(LOGLEVEL_INFO,
                           "\t\tgeneral bar %" PRIu8 ": ignoring\n",
                           index);
                    continue;
                }

                if (result != E_PARSE_BAR_OK) {
                    printk(LOGLEVEL_WARN,
                           "pcie: failed to parse bar %" PRIu8 " result=%d for "
                           "device\n",
                           index,
                           result);
                    break;
                }

                printk(LOGLEVEL_INFO,
                       "\t\tgeneral bar %" PRIu8 " %s: " RANGE_FMT ", %s, %s, "
                       "size: %" PRIu64 "\n",
                       index,
                       bar->is_mmio ? "mmio" : "ports",
                       RANGE_FMT_ARGS(
                        bar->is_mmio ?
                            mmio_region_get_range(bar->mmio) : bar->port_range),
                       bar->is_prefetchable ?
                        "prefetchable" : "not-prefetchable",
                       bar->is_64_bit ? "64-bit" : "32-bit",
                       bar->port_range.size);
            }

            break;
        }
        case PCI_SPEC_DEVHDR_KIND_PCI_BRIDGE: {
            const uint64_t bar_list_size =
                sizeof(struct pci_device_bar_info) * PCI_BAR_COUNT_FOR_BRIDGE;

            info.bar_list = kmalloc(bar_list_size);
            assert_msg(info.bar_list != NULL,
                       "pcie: failed to allocate memory for bar list\n");

            info.max_bar_count = PCI_BAR_COUNT_FOR_BRIDGE;
            pci_parse_capabilities(&info);

            for (uint8_t index = 0;
                 index != PCI_BAR_COUNT_FOR_BRIDGE;
                 index++)
            {
                struct pci_device_bar_info *const bar = info.bar_list + index;
                const enum parse_bar_result result =
                    pci_parse_bar(&info, &index, /*is_bridge=*/true, bar);

                if (result == E_PARSE_BAR_IGNORE) {
                    printk(LOGLEVEL_INFO,
                           "\t\tbridge bar %" PRIu8 ": ignoring\n",
                           index);
                    continue;
                }

                if (result != E_PARSE_BAR_OK) {
                    printk(LOGLEVEL_INFO,
                           "pcie: failed to parse bar %" PRIu8 " result=%d for "
                           "device, " PCI_DEVICE_INFO_FMT "\n",
                            index,
                            result,
                            PCI_DEVICE_INFO_FMT_ARGS(&info));
                    break;
                }

                printk(LOGLEVEL_INFO,
                       "\t\tbridge bar %" PRIu8 " %s: " RANGE_FMT ", %s, %s, "
                       "size: %" PRIu64 "\n",
                       index,
                       bar->is_mmio ? "mmio" : "ports",
                       RANGE_FMT_ARGS(
                        bar->is_mmio ?
                            mmio_region_get_range(bar->mmio) : bar->port_range),
                       bar->is_prefetchable ?
                        "prefetchable" : "not-prefetchable",
                       bar->is_64_bit ? "64-bit" : "32-bit",
                       bar->port_range.size);
            }

            const uint8_t secondary_bus_number =
                group->read(
                    &info,
                    offsetof(struct pci_spec_pci_to_pci_bridge_device_info,
                             secondary_bus_number),
                    sizeof_field(struct pci_spec_pci_to_pci_bridge_device_info,
                                 secondary_bus_number));

            pci_parse_bus(group, &info.config_space, secondary_bus_number);
            break;
        }
        case PCI_SPEC_DEVHDR_KIND_CARDBUS_BRIDGE:
            printk(LOGLEVEL_INFO,
                   "pcie: cardbus bridge not supported. ignoring");
            break;
    }

    list_add(&group->device_list, &info.list);
}

void
pci_parse_bus(struct pci_group *const group,
              struct pci_config_space *const config_space,
              const uint8_t bus)
{
    config_space->bus = bus;
    for (uint8_t slot = 0; slot != PCI_MAX_DEVICE_COUNT; slot++) {
        config_space->device_slot = slot;
        for (uint8_t func = 0; func != PCI_MAX_FUNCTION_COUNT; func++) {
            config_space->function = func;
            parse_function(group, config_space);
        }
    }
}

void pci_parse_group(struct pci_group *const group) {
    printk(LOGLEVEL_DEBUG, "pcie: mmio=%p\n", group->mmio->base);
    struct pci_config_space config_space = {
        .group_segment = group->segment,
        .bus = 0,
        .device_slot = 0,
        .function = 0,
    };

    for (uint16_t i = 0; i != PCI_MAX_BUS_COUNT; i++) {
        pci_parse_bus(group, &config_space, /*bus=*/i);
    }
}

struct pci_group *
pci_group_create_pcie(struct range bus_range,
                      struct mmio_region *const mmio,
                      uint16_t segment)
{
    struct pci_group *const group = kmalloc(sizeof(*group));
    if (group == NULL) {
        return group;
    }

    list_init(&group->list);
    list_init(&group->device_list);

    group->bus_range = bus_range;
    group->mmio = mmio;
    group->segment = segment;

    group->read = pcie_read;
    group->write = pcie_write;

    list_add(&group_list, &group->list);
    group_count++;

    return group;
}

extern uint32_t
pci_read(const struct pci_device_info *dev,
         uint32_t offset,
         uint8_t access_size);

extern bool
pci_write(const struct pci_device_info *dev,
          uint32_t offset,
          uint32_t value,
          uint8_t access_size);

void pci_init() {
    struct pci_device_info dev_0 = {};
    const uint32_t dev_0_first_dword =
        pci_read(&dev_0,
                 offsetof(struct pci_spec_device_info, vendor_id),
                 sizeof_field(struct pci_spec_device_info, vendor_id));

    if (dev_0_first_dword == (uint32_t)-1) {
        printk(LOGLEVEL_WARN, "pci: failed to scan pci bus. aborting init\n");
        return;
    }

    if (get_acpi_info()->mcfg == NULL) {
        struct pci_group *const root_group = kmalloc(sizeof(*root_group));
        assert_msg(root_group != NULL, "failed to allocate pci root group");

        list_init(&root_group->list);
        list_init(&root_group->device_list);

        root_group->read = pci_read;
        root_group->write = pci_write;

        pci_parse_group(root_group);
    } else {
        struct pci_group *group = NULL;
        list_foreach(group, &group_list, list) {
            pci_parse_group(group);
        }
    }
}
