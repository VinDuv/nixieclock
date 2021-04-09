// 150189-71 Nixie Clock alternative firmware
// Distributed under the terms of the MIT license.

// Edit this file to change the time zone and DST settings.

#ifndef SETTINGS_H
#define SETTINGS_H

// Settings for CET/CEST

#define UTC_OFFSET_SECS 3600 // Offset added to UTC to get the standard time

// The following defines should be set to 0 disable DST

#define DST_START_MONTH 3 // Month number for DST start, 1 to 12
#define DST_START_WEEK 5 // Week number for DST start, 1 (first) - 5 (last)
#define DST_START_DAY 6 // Day number for DST start, 0 (Mon) - 6 (Sun)
#define DST_START_HOUR 3 // DST start hour (xx:00:00 DST)

#define DST_END_MONTH 10 // Month number for DST end, 1 to 12
#define DST_END_WEEK 5 // Week number for DST end, 1 (first) - 5 (last)
#define DST_END_DAY 6 // Day number for DST end, 0 (Mon) - 6 (Sun)
#define DST_END_HOUR 3 // DST end hour (xx:00:00 DST)

#endif
