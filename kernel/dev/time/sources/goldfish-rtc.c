/*
 * kernel/dev/time/sources/goldfish-rtc.c
 * © suhas pai
 */

#include "dev/dtb/dtb.h"

#include "dev/driver.h"
#include "dev/printk.h"

#include "lib/align.h"
#include "mm/kmalloc.h"

#include "../kstrftime.h"

#if !defined(__x86_64__)
    #include "port.h"
#endif /* !defined(__x86_64__) */

#include "../time.h"

struct goldfish_rtc {
    volatile uint32_t time_low;
    volatile uint32_t time_high;

    volatile uint32_t alarm_low;
    volatile uint32_t alarm_high;

    volatile uint32_t irq_enabled;

    volatile uint32_t alarm_clear;
    volatile uint32_t alarm_status;

    volatile uint32_t interrupt_clear;
} __packed;

struct goldfish_rtc_info {
    struct clock_source clock_source;
    struct mmio_region *mmio;
};

static uint64_t goldfish_rtc_read(struct clock_source *const clock_source) {
    const struct goldfish_rtc_info *const info =
        container_of(clock_source, struct goldfish_rtc_info, clock_source);

    volatile const struct goldfish_rtc *const rtc = info->mmio->base;
#if defined(__x86_64__)
    const uint64_t lower = rtc->time_low;
    const uint64_t higher = rtc->time_high;
#else
    const uint64_t lower =
        port_in32((port_t)field_to_ptr(struct goldfish_rtc, rtc, time_low));
    const uint64_t higher =
        port_in32((port_t)field_to_ptr(struct goldfish_rtc, rtc, time_high));
#endif

    return (higher << 32 | lower);
}

static
bool goldfish_rtc_init_from_dtb(const void *const dtb, const int nodeoff) {
    struct dtb_addr_size_pair base_addr_reg = {0};
    uint32_t pair_count = 1;

    const bool get_base_addr_reg_result =
        dtb_get_reg_pairs(dtb,
                          nodeoff,
                          /*start_index=*/0,
                          &pair_count,
                          &base_addr_reg);

    if (!get_base_addr_reg_result) {
        printk(LOGLEVEL_WARN,
               "goldfish-rtc: base-addr reg of 'reg' property of "
               "google,goldfish-rtc dtb-node is malformed\n");
        return false;
    }

    if (!has_align(base_addr_reg.address, PAGE_SIZE)) {
        printk(LOGLEVEL_WARN,
               "goldfish-rtc: address is not aligned to page-size\n");
        return false;
    }

    const struct range phys_range =
        range_create(base_addr_reg.address,
                     align_up_assert(base_addr_reg.size, PAGE_SIZE));

    struct mmio_region *const vma =
        vmap_mmio(phys_range, PROT_READ | PROT_WRITE, /*flags=*/0);

    if (vma == NULL) {
        printk(LOGLEVEL_WARN, "goldfish-rtc: failed to mmio-map region\n");
        return false;
    }

    struct goldfish_rtc_info *const clock = kmalloc(sizeof(*clock));
    if (clock == NULL) {
        printk(LOGLEVEL_WARN, "goldfish-rtc: failed to alloc clock-info\n");
        return false;
    }

    list_init(&clock->clock_source.list);

    clock->mmio = vma;
    clock->clock_source.read = goldfish_rtc_read;

    add_clock_source(&clock->clock_source);

    const uint64_t timestamp = clock->clock_source.read(&clock->clock_source);
    printk(LOGLEVEL_INFO,
           "goldfish-rtc: init complete. ns since epoch: %" PRIu64 "\n",
           timestamp);

    const struct tm tm = tm_from_stamp(nano_to_seconds(timestamp));
    struct string string = kstrftime("%c", &tm);

    printk(LOGLEVEL_INFO,
           "goldfish-rtc: current date&time is " STRING_FMT "\n",
           STRING_FMT_ARGS(string));

    string_destroy(&string);
    return true;
}

static const char *const compat[] = { "google,goldfish-rtc" };
__driver static const struct driver driver = {
    .dtb = &(struct dtb_driver){
        .compat_list = compat,
        .compat_count = countof(compat),
        .init = goldfish_rtc_init_from_dtb
    }
};