// 150189-71 Nixie Clock alternative firmware
// Copyright (C) Vincent Duvert
// Distributed under the terms of the MIT license.

#include "datetime.h"

#include <stdbool.h>


#define SECONDS_PER_HOUR 3600UL
#define SECONDS_PER_DAY (24UL * SECONDS_PER_HOUR)
#define NONLEAP_DAYS 365
#define DAYS_PER_FOUR_YEARS (3 * 365 + 366)

// Definition of extern variables
int32_t utc_offset_secs;
struct dst_date dst_start;
struct dst_date dst_end;

struct datetime local_time;

// Day count for non-leap and leap years
static const uint8_t month_days[2][12]  = {
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static uint16_t week_day_to_offset(uint8_t first_day_of_year, bool leap_year,
    uint8_t day_month, uint8_t day_week, uint8_t day_num);

static bool check_dst(uint16_t tstamp_days, uint16_t days_since_new_year,
    uint32_t seconds_in_day, bool leap_year);


// Convert a week day reference ("last Sunday in March") to a day count
// Returns 400 (invalid day count) if month is zero
uint16_t week_day_to_offset(uint8_t first_day_of_year, bool leap_year,
    uint8_t day_month, uint8_t day_week, uint8_t day_num)
{
    uint16_t offset = 0;
    uint8_t cur_month;
    uint8_t first_day_of_month;
    uint8_t month_offset;

    // Go to the first of the month
    for (cur_month = 1 ; cur_month < day_month ; cur_month += 1) {
        offset += month_days[leap_year][cur_month - 1];
    }

    // Find the first day number of the desired month
    first_day_of_month = ((uint16_t)first_day_of_year + offset) % 7;

    // Find the first desired day of the desired month
    month_offset = (day_num - first_day_of_month + 7) % 7;

    // Move to the desired week
    month_offset += 7 * (day_week - 1);

    // 5th week may not fit in month; use 4th week in this case
    if (month_offset >= month_days[leap_year][cur_month - 1]) {
        month_offset -= 7;
    }

    return offset + month_offset;
}


// Check if DST is currently active
bool check_dst(uint16_t tstamp_days, uint16_t days_since_new_year,
    uint32_t seconds_in_day, bool leap_year)
{
    uint16_t new_year_day_offset = tstamp_days - days_since_new_year;
    uint8_t first_day_of_year = (new_year_day_offset + EPOCH_DAY_NUM) % 7;
    uint16_t dst_start_offset;
    uint16_t dst_end_offset;

    if ((dst_start.hour == 0) || (dst_end.hour == 0)
        || (dst_start.month == 0)) {
        return false;
    }

    dst_start_offset = week_day_to_offset(first_day_of_year, leap_year,
        dst_start.month, dst_start.week, dst_start.day);

    dst_end_offset = week_day_to_offset(first_day_of_year, leap_year,
        dst_end.month, dst_end.week, dst_end.day);

    if (days_since_new_year < dst_start_offset) {
        return false;

    } else if (days_since_new_year == dst_start_offset) {
        uint32_t dst_start_second = (uint32_t)(dst_start.hour - 1) *
            SECONDS_PER_HOUR;
        return seconds_in_day >= dst_start_second;

    } else if (days_since_new_year < dst_end_offset) {
        return true;

    } else if (days_since_new_year == dst_end_offset) {
        uint32_t dst_end_second = (uint32_t)(dst_end.hour - 1) *
            SECONDS_PER_HOUR;
        return seconds_in_day < dst_end_second;

    } else {
        return false;
    }
}


// Recalculate the local time from the current timestamp
void recalc_local_time(uint16_t tstamp_days, uint32_t tstamp_secs)
{
    uint16_t remaining_days;
    bool is_leap;
    
    // Adjust timestamp per UTC offset
    if (utc_offset_secs >= 0) {
        tstamp_secs = (uint32_t)((int32_t)tstamp_secs + utc_offset_secs);
        if (tstamp_secs >= SECONDS_PER_DAY) {
            tstamp_days += 1;
            tstamp_secs -= SECONDS_PER_DAY;
        }
    } else {
        if (tstamp_secs < (uint32_t)(-utc_offset_secs)) {
            tstamp_days -= 1;
            tstamp_secs += SECONDS_PER_DAY;
        }
        tstamp_secs = (uint32_t)((int32_t)tstamp_secs + utc_offset_secs);
    }

    remaining_days = tstamp_days;

    // Calculate the current year in local time, and its leap year status
    local_time.year = REF_YEAR + 4 * (remaining_days / DAYS_PER_FOUR_YEARS);
    remaining_days = remaining_days % DAYS_PER_FOUR_YEARS;

    for (;;) {
        uint16_t to_remove;

        is_leap = ((local_time.year % 4) == 0);

        to_remove = is_leap ? (NONLEAP_DAYS + 1) : NONLEAP_DAYS;

        if (remaining_days >= to_remove) {
            local_time.year += 1;
            remaining_days -= to_remove;
        } else {
            break;
        }
    }

    // Adjust timestamp if DST is active
    if (check_dst(tstamp_days, remaining_days, tstamp_secs, is_leap)) {
        tstamp_secs += SECONDS_PER_HOUR;
        if (tstamp_secs >= SECONDS_PER_DAY) {
            remaining_days += 1;
            tstamp_secs -= SECONDS_PER_DAY;
        }
    }

    // Finish formatting the date
    local_time.month = 1;
    while (remaining_days >= month_days[is_leap][local_time.month - 1]) {
        remaining_days -= month_days[is_leap][local_time.month - 1];
        local_time.month += 1;

        if (local_time.month == 13) {
            // Should not happen unless a weird DST was provided.
            local_time.month = 1;
            local_time.year += 1;
            break;
        }
    }

    local_time.day = (uint8_t)remaining_days + 1;

    local_time.hour = (uint8_t)(tstamp_secs / 3600);
    tstamp_secs = tstamp_secs % 3600;
    local_time.minute = (uint8_t)(tstamp_secs / 60);
    local_time.second = tstamp_secs % 60;
}
