/*
 * kernel/arch/x86_64/dev/time/rtc.h
 * © suhas pai
 */

#pragma once
#include "lib/time.h"

struct rtc_time_info {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint64_t year;

    enum weekday weekday;
};

struct rtc_cmos_info {
    struct rtc_time_info time;

    uint8_t reg_status_a;
    uint16_t reg_status_b;
};


void rtc_init();
bool rtc_read_cmos_info(struct rtc_cmos_info *const info_out);
struct tm tm_from_rtc_cmos_info(struct rtc_cmos_info *const info);