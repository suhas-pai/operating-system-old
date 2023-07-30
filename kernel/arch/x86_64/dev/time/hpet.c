/*
 * kernel/arch/x86_64/dev/time/hpet.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/time.h"
#include "mm/mmio.h"

#include "hpet.h"

struct hpet_addrspace_timer_info {
    volatile uint64_t config_and_capability;
    volatile uint64_t comparator_value;
    volatile uint64_t fsb_int_route;
} __packed;

struct hpet_addrspace {
    volatile const uint64_t general_cap_and_id;
    uint64_t padding_1;

    volatile uint64_t general_config;
    uint64_t padding_2;

    volatile uint64_t general_int_status;
    char padding_3[200];

    volatile uint64_t main_counter_value;
    uint64_t padding_4[2];

    struct hpet_addrspace_timer_info timers[31];
} __packed;

static struct mmio_region *hpet_mmio = NULL;
void hpet_init(const struct acpi_hpet *const hpet) {
    if (hpet->base_address.addr_space != ACPI_GAS_ADDRSPACE_KIND_SYSMEM) {
        printk(LOGLEVEL_WARN,
               "hpet: address space is not system-memory. init failed\n");
        return;
    }

    const bool has_64bit_counter =
        (hpet->event_timer_block_id &
           __HPET_EVENTTIMER_BLOCKID_64BIT_COUNTER) != 0;

    if (!has_64bit_counter) {
        printk(LOGLEVEL_WARN,
               "hpet: does not support 64-bit counters. aborting init");
        return;
    }

    hpet_mmio =
        vmap_mmio(range_create(hpet->base_address.address, PAGE_SIZE),
                               PROT_READ | PROT_WRITE,
                               /*flags=*/0);

    if (hpet_mmio == NULL) {
        printk(LOGLEVEL_WARN, "hpet: failed to mmio-map address-space\n");
        return;
    }

    volatile struct hpet_addrspace *const addrspace =
        (volatile struct hpet_addrspace *)hpet_mmio->base;

    const uint64_t cap_and_id = addrspace->general_cap_and_id;
    const uint32_t main_counter_period = cap_and_id >> 32;

    printk(LOGLEVEL_INFO,
           "hpet: period is %" PRIu32 ".%" PRIu32 " nanoseconds\n",
           femto_to_nano(main_counter_period),
           femto_mod_nano(main_counter_period));

    addrspace->general_config = 0;

    const uint8_t timer_count = (cap_and_id >> 8) & 0x1f;
    printk(LOGLEVEL_WARN, "hpet: got %" PRIu8 " timers\n", timer_count);

    for (volatile struct hpet_addrspace_timer_info *timer = addrspace->timers;
         timer != addrspace->timers + timer_count;
         timer++)
    {
        timer->comparator_value = 0;
    }

    addrspace->main_counter_value = 1;
    addrspace->general_config = 1;
}