/*
 * include/lib/time.h
 * © suhas pai
 */

#pragma once

#include <stdbool.h>

#include "adt/string_view.h"
#include "lib/macros.h"

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

enum weekday weekday_get_previous_weekday(enum weekday weekday);
enum weekday weekday_get_following_weekday(enum weekday weekday);

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