/*
 * kernel/arch/aarch64/dev/time/time.c
 * © suhas pai
 */

#include "dev/printk.h"
#include "lib/time.h"

#include "limine.h"

static volatile struct limine_boot_time_request boot_time_request = {
    .id = LIMINE_BOOT_TIME_REQUEST,
    .revision = 0
};

static uint64_t boot_nsec = 0;
uint64_t nsec_since_boot() {
    return boot_nsec;
}

void arch_init_time() {
    assert_msg(boot_time_request.response != NULL,
               "time: bootloader did not provide boot time");

    assert_msg(boot_time_request.response->boot_time > 0,
               "time: bootloader's boot-time is 0 or negative");

    boot_nsec =
        seconds_to_nano((uint64_t)boot_time_request.response->boot_time);
}