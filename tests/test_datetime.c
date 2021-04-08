#include <stdio.h>

#include "datetime.h"


static int exit_status = 0;


static void test_date_calc(uint16_t days, uint16_t exp_year, uint8_t exp_month, uint8_t exp_day)
{
    utc_offset_secs = 0;

    recalc_local_time(days, 0);

    uint16_t year = local_time.year;
    uint8_t month = local_time.month;
    uint8_t day = local_time.day;

    if (year == exp_year && month == exp_month && day == exp_day) {
        printf("OK %hu => %02hhu/%02hhu/%04hu\n", days, day, month, year);
    } else {
        printf("KO %hu => %02hhu/%02hhu/%04hu (expected %02hhu/%02hhu/%04hu)\n",
            days, day, month, year, exp_day, exp_month, exp_year);
        exit_status = 1;
    }
}


static void run_date_calc_tests(void)
{
    test_date_calc(0, 1970, 1, 1);
    test_date_calc(59, 1970, 3, 1);
    test_date_calc(364, 1970, 12, 31);

    test_date_calc(365, 1971, 1, 1);
    test_date_calc(365 + 59, 1971, 3, 1);
    test_date_calc(365 + 364, 1971, 12, 31);

    test_date_calc(730, 1972, 1, 1);
    test_date_calc(730 + 59, 1972, 2, 29);
    test_date_calc(730 + 365, 1972, 12, 31);

    test_date_calc(1096, 1973, 1, 1);
    test_date_calc(1096 + 59, 1973, 3, 1);
    test_date_calc(1096 + 364, 1973, 12, 31);

    test_date_calc(2191, 1976, 1, 1);
    test_date_calc(2191 + 59, 1976, 2, 29);
    test_date_calc(2191 + 365, 1976, 12, 31);

    test_date_calc(2557, 1977, 1, 1);
    test_date_calc(2557 + 59, 1977, 3, 1);
    test_date_calc(2557 + 364, 1977, 12, 31);

    test_date_calc(10957, 2000, 1, 1);
    test_date_calc(10957 + 59, 2000, 2, 29);
    test_date_calc(10957 + 365, 2000, 12, 31);

    test_date_calc(11323, 2001, 1, 1);
    test_date_calc(11323 + 59, 2001, 3, 1);
    test_date_calc(11323 + 364, 2001, 12, 31);
}


static void test_dst_calc(uint32_t seconds_from_epoch, uint16_t exp_year,
    uint8_t exp_month, uint8_t exp_day, uint8_t exp_hour, uint8_t exp_min,
    uint8_t exp_sec) {

    uint16_t tstamp_days = seconds_from_epoch / 86400U;
    uint32_t tstamp_ticks = (uint32_t)seconds_from_epoch % 86400U;

    recalc_local_time(tstamp_days, tstamp_ticks);

    uint16_t year = local_time.year;
    uint8_t month = local_time.month;
    uint8_t day = local_time.day;
    uint8_t hour = local_time.hour;
    uint8_t minute = local_time.minute;
    uint8_t second = local_time.second;

    if (year == exp_year && month == exp_month && day == exp_day &&
        hour == exp_hour && minute == exp_min && second == exp_sec) {
        printf("OK %u => %02hhu/%02hhu/%04hu %02hhu:%02hhu:%02hhu\n",
            seconds_from_epoch, day, month, year, hour, minute, second);
    } else {
        printf("KO %u => %02hhu/%02hhu/%04hu %02hhu:%02hhu:%02hhu "
            "(expected %02hhu/%02hhu/%04hu %02hhu:%02hhu:%02hhu)\n",
            seconds_from_epoch, day, month, year, hour, minute, second,
            exp_day, exp_month, exp_year, exp_hour, exp_min, exp_sec);
        exit_status = 1;
    }
}


static void run_dst_tests(void)
{
    // CET/CEST transition
    utc_offset_secs = 3600;
    dst_start.month = 3;
    dst_start.week = 5;
    dst_start.day = 6;
    dst_start.hour = 3;

    dst_end.month = 10;
    dst_end.week = 5;
    dst_end.day = 6;
    dst_end.hour = 3;

    // Start of the year
    test_dst_calc(1609455600, 2021, 1, 1, 0, 0, 0);

    // Just before DST starts
    test_dst_calc(1616893199, 2021, 3, 28, 1, 59, 59);

    // Just after DST starts (2:00 -> 3:00)
    test_dst_calc(1616893200, 2021, 3, 28, 3, 0, 0);

    // Next (non-DST) day after DST starts
    test_dst_calc(1616976000, 2021, 3, 29, 2, 0, 0);

    // Just before DST ends
    test_dst_calc(1635641999, 2021, 10, 31, 2, 59, 59);

    // Just after DST ends (3:00 -> 2:00)
    test_dst_calc(1635642000, 2021, 10, 31, 2, 0, 0);

    // Last second of year
    test_dst_calc(1640991599, 2021, 12, 31, 23, 59, 59);

    // Degraded case: DST still active at the end of the year
    utc_offset_secs = 0;
    dst_start.month = 3;
    dst_start.week = 5;
    dst_start.day = 6;
    dst_start.hour = 3;

    dst_end.month = 12;
    dst_end.week = 5;
    dst_end.day = 3;
    dst_end.hour = 25;

    test_dst_calc(1609455600, 2021, 1, 1, 0, 0, 0);

}


int main(void)
{
    run_date_calc_tests();
    run_dst_tests();

    return exit_status;
}
