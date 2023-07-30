/*
 * include/lib/time.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>

#include "adt/string_view.h"
#include "lib/macros.h"

#define femto_to_pico(femto) ((femto) / 1000)
#define pico_to_nano(pico) ((pico) / 1000)
#define nano_to_micro(nano) ((nano) / 1000)
#define micro_to_milli(micro) ((micro) / 1000)
#define milli_to_seconds(milli) ((milli) / 1000)

#define seconds_to_minutes(seconds) ((seconds) / 60)
#define minutes_to_hours(minutes) ((minutes) / 60)
#define hours_to_days(minutes) ((hours) / 24)

#define femto_to_nano(femto) pico_to_nano(femto_to_pico(femto))
#define pico_to_micro(pico) nano_to_micro(pico_to_nano(pico))
#define nano_to_milli(nano) micro_to_milli(nano_to_micro(nano))
#define micro_to_seconds(micro) milli_to_seconds(micro_to_milli(micro))
#define milli_to_minutes(milli) seconds_to_minutes(milli_to_seconds(micro))
#define seconds_to_hours(seconds) minutes_to_hours(seconds_to_minutes(seconds))
#define minutes_to_days(minutes) hours_to_days(minutes_to_hours(minutes))

#define femto_to_micro(femto) nano_to_micro(femto_to_nano(femto))
#define pico_to_milli(pico) micro_to_milli(pico_to_micro(pico))
#define nano_to_seconds(nano) milli_to_seconds(nano_to_milli(nano))
#define micro_to_minutes(micro) seconds_to_minutes(micro_to_seconds(micro))
#define milli_to_hours(milli) minutes_to_hours(milli_to_minutes(milli))
#define seconds_to_days(seconds) hours_to_days(seconds_to_hours(seconds))

#define femto_to_milli(femto) nano_to_milli(femto_to_nano(femto))
#define pico_to_seconds(pico) milli_to_seconds(pico_to_milli(pico))
#define nano_to_minutes(nano) seconds_to_minutes(nano_to_seconds(nano))
#define micro_to_hours(micro) minutes_to_hours(micro_to_minutes(milli))
#define milli_to_days(milli) hours_to_days(milli_to_hours(milli))

#define femto_to_seconds(femto) milli_to_seconds(femto_to_milli(femto))
#define pico_to_minutes(pico) seconds_to_minutes(pico_to_seconds(pico))
#define nano_to_hours(nano) minutes_to_hours(nano_to_minutes(nano))
#define micro_to_days(micro) hours_to_days(micro_to_hours(micro))

#define femto_to_minutes(femto) seconds_to_minutes(femto_to_seconds(femto))
#define pico_to_hours(pico) minutes_to_hours(pico_to_minutes(pico))
#define nano_to_days(nano) hours_to_days(nano_to_hours(nano))

#define femto_to_hours(femto) minutes_to_hours(femto_to_minutes(femto))
#define pico_to_days(pico) hours_to_days(pico_to_hours(pico))

#define femto_to_days(femto) hours_to_days(femto_to_hours(femto))

#define pico_to_femto(pico) ((pico) * 1000)
#define nano_to_pico(nano) ((nano) * 1000)
#define micro_to_nano(micro) ((micro) * 1000)
#define milli_to_micro(milli) ((milli) * 1000)
#define seconds_to_milli(seconds) ((seconds) * 1000)

#define minutes_to_seconds(minutes) ((minutes) * 60)
#define hours_to_minutes(hours) ((hours) * 60)
#define days_to_hours(days) ((days) * 24)

#define nano_to_femto(femto) pico_to_femto(nano_to_pico(femto))
#define micro_to_pico(pico) nano_to_pico(micro_to_nano(pico))
#define milli_to_nano(nano) micro_to_nano(milli_to_micro(nano))
#define seconds_to_micro(micro) milli_to_micro(seconds_to_milli(micro))
#define minutes_to_milli(minutes) seconds_to_milli(minutes_to_seconds(minutes))
#define hours_to_seconds(hours) minutes_to_seconds(hours_to_minutes(hours))
#define days_to_minutes(days) hours_to_minutes(days_to_hours(days))

#define micro_to_femto(micro) nano_to_femto(micro_to_nano(femto))
#define milli_to_pico(milli) micro_to_pico(milli_to_micro(pico))
#define seconds_to_nano(seconds) milli_to_nano(seconds_to_milli(nano))
#define minutes_to_micro(minutes) seconds_to_micro(minutes_to_seconds(micro))
#define hours_to_milli(hours) minutes_to_micro(hours_to_minutes(micro))
#define days_to_seconds(days) hours_to_seconds(days_to_hours(micro))

#define milli_to_femto(milli) nano_to_femto(milli_to_nano(milli))
#define seconds_to_pico(seconds) milli_to_pico(seconds_to_milli(seconds))
#define minutes_to_nano(minutes) seconds_to_nano(minutes_to_seconds(minutes))
#define hours_to_micro(hours) minutes_to_micro(hours_to_minutes(hours))
#define days_to_milli(days) hours_to_milli(days_to_hours(days))

#define seconds_to_femto(seconds) milli_to_femto(seconds_to_milli(seconds))
#define minutes_to_pico(minutes) seconds_to_pico(minutes_to_seconds(minutes))
#define hours_to_nano(hours) minutes_to_nano(hours_to_minutes(hours))
#define days_to_micro(days) hours_to_micro(days_to_hours(days))

#define minutes_to_femto(minutes) seconds_to_femto(minutes_to_seconds(minutes))
#define hours_to_pico(hours) minutes_to_pico(hours_to_minutes(hours))
#define days_to_nano(days) hours_to_nano(days_to_hours(days))

#define hours_to_femto(hours) minutes_to_femto(hours_to_minutes(hours))
#define days_to_pico(days) hours_to_pico(days_to_hours(days))

#define days_to_femto(femto) hours_to_femto(days_to_hours(femto))

#define femto_mod_pico(femto) ((femto) % 1000)
#define pico_mod_nano(pico) ((pico) % 1000)
#define nano_mod_micro(nano) ((nano) % 1000)
#define micro_mod_milli(micro) ((micro) % 1000)
#define milli_mod_seconds(milli) ((milli) % 1000)

#define seconds_mod_minutes(seconds) ((seconds) % 60)
#define minutes_mod_hours(minutes) ((minutes) % 60)
#define hours_mod_days(minutes) ((hours) % 24)

#define femto_mod_nano(femto) pico_mod_nano(femto_mod_pico(femto))
#define pico_mod_micro(pico) nano_mod_micro(pico_mod_nano(pico))
#define nano_mod_milli(nano) micro_mod_milli(nano_mod_micro(nano))
#define micro_mod_seconds(micro) milli_mod_seconds(micro_mod_milli(micro))
#define milli_mod_minutes(milli) seconds_mod_minutes(milli_mod_seconds(micro))
#define seconds_mod_hours(seconds) minutes_mod_hours(seconds_mod_minutes(seconds))
#define minutes_mod_days(minutes) hours_mod_days(minutes_mod_hours(minutes))

#define femto_mod_micro(femto) nano_mod_micro(femto_mod_nano(femto))
#define pico_mod_milli(pico) micro_mod_milli(pico_mod_micro(pico))
#define nano_mod_seconds(nano) milli_mod_seconds(nano_mod_milli(nano))
#define micro_mod_minutes(micro) seconds_mod_minutes(micro_mod_seconds(micro))
#define milli_mod_hours(milli) minutes_mod_hours(milli_mod_minutes(milli))
#define seconds_mod_days(seconds) hours_mod_days(seconds_mod_hours(seconds))

#define femto_mod_milli(femto) nano_mod_milli(femto_mod_nano(femto))
#define pico_mod_seconds(pico) milli_mod_seconds(pico_mod_milli(pico))
#define nano_mod_minutes(nano) seconds_mod_minutes(nano_mod_seconds(nano))
#define micro_mod_hours(micro) minutes_mod_hours(micro_mod_minutes(milli))
#define milli_mod_days(milli) hours_mod_days(milli_mod_hours(milli))

#define femto_mod_seconds(femto) milli_mod_seconds(femto_mod_milli(femto))
#define pico_mod_minutes(pico) seconds_mod_minutes(pico_mod_seconds(pico))
#define nano_mod_hours(nano) minutes_mod_hours(nano_mod_minutes(nano))
#define micro_mod_days(micro) hours_mod_days(micro_mod_hours(micro))

#define femto_mod_minutes(femto) seconds_mod_minutes(femto_mod_seconds(femto))
#define pico_mod_hours(pico) minutes_mod_hours(pico_mod_minutes(pico))
#define nano_mod_days(nano) hours_mod_days(nano_mod_hours(nano))

#define femto_mod_hours(femto) minutes_mod_hours(femto_mod_minutes(femto))
#define pico_mod_days(pico) hours_mod_days(pico_mod_hours(pico))

#define femto_mod_days(femto) hours_mod_days(femto_mod_hours(femto))

enum weekday {
    WEEKDAY_INVALID = -1,

    WEEKDAY_SUNDAY = 0,
    WEEKDAY_MONDAY,
    WEEKDAY_TUESDAY,
    WEEKDAY_WEDNESDAY,
    WEEKDAY_THURSDAY,
    WEEKDAY_FRIDAY,
    WEEKDAY_SATURDAY,
};

enum month {
    MONTH_INVALID,

    MONTH_JANUARY = 1,
    MONTH_FEBRUARY,
    MONTH_MARCH,
    MONTH_APRIL,
    MONTH_MAY,
    MONTH_JUNE,
    MONTH_JULY,
    MONTH_AUGUST,
    MONTH_SEPTEMBER,
    MONTH_OCTOBER,
    MONTH_NOVEMBER,
    MONTH_DECEMBER
};

/* Stop colliding with Apple's time.h */
#ifndef _TIME_H_
    struct tm {
        int tm_sec;
        int tm_min;
        int tm_hour;
        int tm_mday;
        int tm_mon;
        int tm_year;
        int tm_wday;
        int tm_yday;
        int tm_isdst;
    };
#endif /* _TIME_H_ */

#define C_STR_JANUARY   "January"
#define C_STR_FEBRUARY  "February"
#define C_STR_MARCH     "March"
#define C_STR_APRIL     "April"
#define C_STR_MAY       "May"
#define C_STR_JUNE      "June"
#define C_STR_JULY      "July"
#define C_STR_AUGUST    "August"
#define C_STR_SEPTEMBER "September"
#define C_STR_OCTOBER   "October"
#define C_STR_NOVEMBER  "November"
#define C_STR_DECEMBER  "December"

#define C_STR_JANUARY_UPPER   "JANUARY"
#define C_STR_FEBRUARY_UPPER  "FEBRUARY"
#define C_STR_MARCH_UPPER     "MARCH"
#define C_STR_APRIL_UPPER     "APRIL"
#define C_STR_MAY_UPPER       "MAY"
#define C_STR_JUNE_UPPER      "JUNE"
#define C_STR_JULY_UPPER      "JULY"
#define C_STR_AUGUST_UPPER    "AUGUST"
#define C_STR_SEPTEMBER_UPPER "SEPTEMBER"
#define C_STR_OCTOBER_UPPER   "OCTOBER"
#define C_STR_NOVEMBER_UPPER  "NOVEMBER"
#define C_STR_DECEMBER_UPPER  "DECEMBER"

#define C_STR_JANUARY_ABBREV   "Jan"
#define C_STR_FEBRUARY_ABBREV  "Feb"
#define C_STR_MARCH_ABBREV     "Mar"
#define C_STR_APRIL_ABBREV     "Apr"
#define C_STR_MAY_ABBREV       "May"
#define C_STR_JUNE_ABBREV      "Jun"
#define C_STR_JULY_ABBREV      "Jul"
#define C_STR_AUGUST_ABBREV    "Aug"
#define C_STR_SEPTEMBER_ABBREV "Sep"
#define C_STR_OCTOBER_ABBREV   "Oct"
#define C_STR_NOVEMBER_ABBREV  "Nov"
#define C_STR_DECEMBER_ABBREV  "Dec"

#define C_STR_JANUARY_ABBREV_UPPER   "JAN"
#define C_STR_FEBRUARY_ABBREV_UPPER  "FEB"
#define C_STR_MARCH_ABBREV_UPPER     "MAR"
#define C_STR_APRIL_ABBREV_UPPER     "APR"
#define C_STR_MAY_ABBREV_UPPER       "MAY"
#define C_STR_JUNE_ABBREV_UPPER      "JUN"
#define C_STR_JULY_ABBREV_UPPER      "JUL"
#define C_STR_AUGUST_ABBREV_UPPER    "AUG"
#define C_STR_SEPTEMBER_ABBREV_UPPER "SEP"
#define C_STR_OCTOBER_ABBREV_UPPER   "OCT"
#define C_STR_NOVEMBER_ABBREV_UPPER  "NOV"
#define C_STR_DECEMBER_ABBREV_UPPER  "DEC"

#define C_STR_SUNDAY    "Sunday"
#define C_STR_MONDAY    "Monday"
#define C_STR_TUESDAY   "Tuesday"
#define C_STR_WEDNESDAY "Wednesday"
#define C_STR_THURSDAY  "Thursday"
#define C_STR_FRIDAY    "Friday"
#define C_STR_SATURDAY  "Saturday"

#define C_STR_SUNDAY_UPPER    "SUNDAY"
#define C_STR_MONDAY_UPPER    "MONDAY"
#define C_STR_TUESDAY_UPPER   "TUESDAY"
#define C_STR_WEDNESDAY_UPPER "WEDNESDAY"
#define C_STR_THURSDAY_UPPER  "THURSDAY"
#define C_STR_FRIDAY_UPPER    "FRIDAY"
#define C_STR_SATURDAY_UPPER  "SATURDAY"

#define C_STR_SUNDAY_ABBREV    "Sun"
#define C_STR_MONDAY_ABBREV    "Mon"
#define C_STR_TUESDAY_ABBREV   "Tue"
#define C_STR_WEDNESDAY_ABBREV "Wed"
#define C_STR_THURSDAY_ABBREV  "Thu"
#define C_STR_FRIDAY_ABBREV    "Fri"
#define C_STR_SATURDAY_ABBREV  "Sat"

#define C_STR_SUNDAY_ABBREV_UPPER    "SUN"
#define C_STR_MONDAY_ABBREV_UPPER    "MON"
#define C_STR_TUESDAY_ABBREV_UPPER   "TUE"
#define C_STR_WEDNESDAY_ABBREV_UPPER "WED"
#define C_STR_THURSDAY_ABBREV_UPPER  "THU"
#define C_STR_FRIDAY_ABBREV_UPPER    "FRI"
#define C_STR_SATURDAY_ABBREV_UPPER  "SAT"

#define SV_JANUARY   SV_STATIC(C_STR_JANUARY)
#define SV_FEBRUARY  SV_STATIC(C_STR_FEBRUARY)
#define SV_MARCH     SV_STATIC(C_STR_MARCH)
#define SV_APRIL     SV_STATIC(C_STR_APRIL)
#define SV_MAY       SV_STATIC(C_STR_MAY)
#define SV_JUNE      SV_STATIC(C_STR_JUNE)
#define SV_JULY      SV_STATIC(C_STR_JULY)
#define SV_AUGUST    SV_STATIC(C_STR_AUGUST)
#define SV_SEPTEMBER SV_STATIC(C_STR_SEPTEMBER)
#define SV_OCTOBER   SV_STATIC(C_STR_OCTOBER)
#define SV_NOVEMBER  SV_STATIC(C_STR_NOVEMBER)
#define SV_DECEMBER  SV_STATIC(C_STR_DECEMBER)

#define SV_JANUARY_UPPER   SV_STATIC(C_STR_JANUARY_UPPER)
#define SV_FEBRUARY_UPPER  SV_STATIC(C_STR_FEBRUARY_UPPER)
#define SV_MARCH_UPPER     SV_STATIC(C_STR_MARCH_UPPER)
#define SV_APRIL_UPPER     SV_STATIC(C_STR_APRIL_UPPER)
#define SV_MAY_UPPER       SV_STATIC(C_STR_MAY_UPPER)
#define SV_JUNE_UPPER      SV_STATIC(C_STR_JUNE_UPPER)
#define SV_JULY_UPPER      SV_STATIC(C_STR_JULY_UPPER)
#define SV_AUGUST_UPPER    SV_STATIC(C_STR_AUGUST_UPPER)
#define SV_SEPTEMBER_UPPER SV_STATIC(C_STR_SEPTEMBER_UPPER)
#define SV_OCTOBER_UPPER   SV_STATIC(C_STR_OCTOBER_UPPER)
#define SV_NOVEMBER_UPPER  SV_STATIC(C_STR_NOVEMBER_UPPER)
#define SV_DECEMBER_UPPER  SV_STATIC(C_STR_DECEMBER_UPPER)

#define SV_JANUARY_ABBREV   SV_STATIC(C_STR_JANUARY_ABBREV)
#define SV_FEBRUARY_ABBREV  SV_STATIC(C_STR_FEBRUARY_ABBREV)
#define SV_MARCH_ABBREV     SV_STATIC(C_STR_MARCH_ABBREV)
#define SV_APRIL_ABBREV     SV_STATIC(C_STR_APRIL_ABBREV)
#define SV_MAY_ABBREV       SV_STATIC(C_STR_MAY_ABBREV)
#define SV_JUNE_ABBREV      SV_STATIC(C_STR_JUNE_ABBREV)
#define SV_JULY_ABBREV      SV_STATIC(C_STR_JULY_ABBREV)
#define SV_AUGUST_ABBREV    SV_STATIC(C_STR_AUGUST_ABBREV)
#define SV_SEPTEMBER_ABBREV SV_STATIC(C_STR_SEPTEMBER_ABBREV)
#define SV_OCTOBER_ABBREV   SV_STATIC(C_STR_OCTOBER_ABBREV)
#define SV_NOVEMBER_ABBREV  SV_STATIC(C_STR_NOVEMBER_ABBREV)
#define SV_DECEMBER_ABBREV  SV_STATIC(C_STR_DECEMBER_ABBREV)

#define SV_JANUARY_ABBREV_UPPER   SV_STATIC(C_STR_JANUARY_ABBREV_UPPER)
#define SV_FEBRUARY_ABBREV_UPPER  SV_STATIC(C_STR_FEBRUARY_ABBREV_UPPER)
#define SV_MARCH_ABBREV_UPPER     SV_STATIC(C_STR_MARCH_ABBREV_UPPER)
#define SV_APRIL_ABBREV_UPPER     SV_STATIC(C_STR_APRIL_ABBREV_UPPER)
#define SV_MAY_ABBREV_UPPER       SV_STATIC(C_STR_MAY_ABBREV_UPPER)
#define SV_JUNE_ABBREV_UPPER      SV_STATIC(C_STR_JUNE_ABBREV_UPPER)
#define SV_JULY_ABBREV_UPPER      SV_STATIC(C_STR_JULY_ABBREV_UPPER)
#define SV_AUGUST_ABBREV_UPPER    SV_STATIC(C_STR_AUGUST_ABBREV_UPPER)
#define SV_SEPTEMBER_ABBREV_UPPER SV_STATIC(C_STR_SEPTEMBER_ABBREV_UPPER)
#define SV_OCTOBER_ABBREV_UPPER   SV_STATIC(C_STR_OCTOBER_ABBREV_UPPER)
#define SV_NOVEMBER_ABBREV_UPPER  SV_STATIC(C_STR_NOVEMBER_ABBREV_UPPER)
#define SV_DECEMBER_ABBREV_UPPER  SV_STATIC(C_STR_DECEMBER_ABBREV_UPPER)

#define SV_SUNDAY    SV_STATIC(C_STR_SUNDAY)
#define SV_MONDAY    SV_STATIC(C_STR_MONDAY)
#define SV_TUESDAY   SV_STATIC(C_STR_TUESDAY)
#define SV_WEDNESDAY SV_STATIC(C_STR_WEDNESDAY)
#define SV_THURSDAY  SV_STATIC(C_STR_THURSDAY)
#define SV_FRIDAY    SV_STATIC(C_STR_FRIDAY)
#define SV_SATURDAY  SV_STATIC(C_STR_SATURDAY)

#define SV_SUNDAY_UPPER    SV_STATIC(C_STR_SUNDAY_UPPER)
#define SV_MONDAY_UPPER    SV_STATIC(C_STR_MONDAY_UPPER)
#define SV_TUESDAY_UPPER   SV_STATIC(C_STR_TUESDAY_UPPER)
#define SV_WEDNESDAY_UPPER SV_STATIC(C_STR_WEDNESDAY_UPPER)
#define SV_THURSDAY_UPPER  SV_STATIC(C_STR_THURSDAY_UPPER)
#define SV_FRIDAY_UPPER    SV_STATIC(C_STR_FRIDAY_UPPER)
#define SV_SATURDAY_UPPER  SV_STATIC(C_STR_SATURDAY_UPPER)

#define SV_SUNDAY_ABBREV    SV_STATIC(C_STR_SUNDAY_ABBREV)
#define SV_MONDAY_ABBREV    SV_STATIC(C_STR_MONDAY_ABBREV)
#define SV_TUESDAY_ABBREV   SV_STATIC(C_STR_TUESDAY_ABBREV)
#define SV_WEDNESDAY_ABBREV SV_STATIC(C_STR_WEDNESDAY_ABBREV)
#define SV_THURSDAY_ABBREV  SV_STATIC(C_STR_THURSDAY_ABBREV)
#define SV_FRIDAY_ABBREV    SV_STATIC(C_STR_FRIDAY_ABBREV)
#define SV_SATURDAY_ABBREV  SV_STATIC(C_STR_SATURDAY_ABBREV)

#define SV_SUNDAY_ABBREV_UPPER    SV_STATIC(C_STR_SUNDAY_ABBREV_UPPER)
#define SV_MONDAY_ABBREV_UPPER    SV_STATIC(C_STR_MONDAY_ABBREV_UPPER)
#define SV_TUESDAY_ABBREV_UPPER   SV_STATIC(C_STR_TUESDAY_ABBREV_UPPER)
#define SV_WEDNESDAY_ABBREV_UPPER SV_STATIC(C_STR_WEDNESDAY_ABBREV_UPPER)
#define SV_THURSDAY_ABBREV_UPPER  SV_STATIC(C_STR_THURSDAY_ABBREV_UPPER)
#define SV_FRIDAY_ABBREV_UPPER    SV_STATIC(C_STR_FRIDAY_ABBREV_UPPER)
#define SV_SATURDAY_ABBREV_UPPER  SV_STATIC(C_STR_SATURDAY_ABBREV_UPPER)

#define LONGEST_WEEKDAY_C_STR C_STR_WEDNESDAY
#define LONGEST_WEEKDAY_LENGTH LEN_OF(LONGEST_WEEKDAY_C_STR)

#define LONGEST_MONTH_C_STR C_STR_SEPTEMBER
#define LONGEST_MONTH_LENGTH LEN_OF(LONGEST_MONTH_C_STR)

#define MAX_SECOND_NUMBER_LENGTH LEN_OF("59")
#define MAX_MINUTE_NUMBER_LENGTH LEN_OF("59")

#define MAX_HOUR_NUMBER_LENGTH LEN_OF("23")
#define MAX_YEAR_DAY_NUMBER_LENGTH LEN_OF("365")

#define WEEKDAY_COUNT 7
#define MONTH_COUNT 12

#define MAX_DAY_OF_MONTH_NUMBER_LENGTH LEN_OF("31")
#define MAX_MONTH_NUMBER_LENGTH LEN_OF(TO_STRING(MONTH_COUNT))
#define MAX_WEEK_NUMBER_LENGTH LEN_OF("53")
#define MAX_WEEKDAY_NUMBER_LENGTH LEN_OF("7")

/*
 * Weekday - Day of the Week (Sunday...Saturday)
 * Year day - Days since January 1st
 */

static inline bool weekday_is_valid(const enum weekday day) {
    return day >= WEEKDAY_SUNDAY && day <= WEEKDAY_SATURDAY;
}

static inline bool month_is_valid(const enum month month) {
    return month >= MONTH_JANUARY && month <= MONTH_DECEMBER;
}

uint8_t hour_12_to_24hour(uint8_t hour, bool is_pm);
uint8_t hour_24_to_12hour(uint8_t hour);

bool hour_24_is_pm(uint8_t hour);
uint8_t weekday_to_decimal_monday_one(enum weekday weekday);

uint8_t month_get_day_count(enum month month, bool in_leap_year);
uint8_t
get_week_count_at_day(enum weekday weekday,
                      uint16_t days_since_jan_1,
                      bool is_monday_first);

enum month tm_mon_to_month(int tm_mon);
bool year_is_leap_year(uint64_t year);
uint16_t year_get_day_count(uint64_t year);
int year_to_tm_year(uint64_t year);
uint64_t tm_year_to_year(int tm_year);

enum weekday weekday_prev(enum weekday weekday);
enum weekday weekday_next(enum weekday weekday);

struct string_view weekday_to_sv(enum weekday day);
struct string_view month_to_sv(enum month month);

struct string_view weekday_to_sv_upper(enum weekday day);
struct string_view month_to_sv_upper(enum month month);

struct string_view weekday_to_sv_abbrev(enum weekday day);
struct string_view month_to_sv_abbrev(enum month month);

struct string_view weekday_to_sv_abbrev_upper(enum weekday day);
struct string_view month_to_sv_abbrev_upper(enum month month);

enum weekday sv_to_weekday(struct string_view sv);
enum month sv_to_month(struct string_view sv);

enum weekday sv_abbrev_to_weekday(struct string_view sv);
enum month sv_abbrev_to_month(struct string_view sv);

enum weekday
day_of_month_to_weekday(uint64_t year, enum month month, uint8_t day);

uint16_t
day_of_month_to_day_of_year(uint8_t day, enum month month, bool in_leap_year);

uint8_t
iso_8601_get_week_number(enum weekday weekday,
                         enum month month,
                         uint64_t year,
                         uint8_t day_in_month,
                         uint16_t days_since_jan_1);