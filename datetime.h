// 150189-71 Nixie Clock alternative firmware
// Distributed under the terms of the MIT license.

#ifndef DATETIME_H
#define DATETIME_H

#include <stdint.h>

// Reference year of the timestamp. A timestamp of 0 is 1/1/<year> 00:00:00 UTC
#define REF_YEAR 1970

// Day number of 1/1/<ref year>. 0 = Monday, 6 = Sunday
#define EPOCH_DAY_NUM 3

// DST configuration
struct dst_date {
    uint8_t month; // Month, 1-12, 0 to disable DST
    uint8_t week; // Week in month, 1-4, 5 for last week in month
    uint8_t day; // Day number in week, 0 = Monday, 6 = Sunday
    uint8_t hour; // Start/end hour (xx:00:00 DST)
};

// Local date and time
struct datetime {
    uint16_t year; // 4-digit
    uint8_t month; // 1 - 12
    uint8_t day; // 1 - 31
    uint8_t hour; // 0 - 23
    uint8_t minute; // 0 - 59
    uint8_t second; // 0 - 59
};

// The following variables are set externally:
// Offset from UTC, in seconds; added to timestamp to get local time w/o DST
extern int32_t utc_offset_secs;
extern struct dst_date dst_start; // DST start date
extern struct dst_date dst_end; // DST end date

// Recalculate the local date/time from the current timestamp
// timestamp = tstamp_days * 86400 + tstamp_secs
// tstamp_secs < 86400
void recalc_local_time(uint16_t tstamp_days, uint32_t tstamp_secs);

// The following variables are calculated by recalc_local_time
extern struct datetime local_time;

#endif
