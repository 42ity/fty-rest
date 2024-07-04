/*
 *
 * Copyright (C) 2014 - 2020 Eaton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "shared/utils.h"

#include <fty_common.h>
#include <mutex>
#include <cmath>
#include <errno.h>

static std::mutex timegm_mux;  // Mutex for my_timegm function which is not thread-safe

bool is_average_step_supported(const char* step)
{
    if (!step) {
        return false;
    }
    for (int i = 0; i < AVG_STEPS_SIZE; ++i) {
        if (strcmp(step, AVG_STEPS[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool is_average_type_supported(const char* type)
{
    if (!type) {
        return false;
    }
    for (int i = 0; i < AVG_TYPES_SIZE; ++i) {
        if (strcmp(type, AVG_TYPES[i]) == 0) {
            return true;
        }
    }
    return false;
}

int64_t average_step_seconds(const char* step)
{
    if (!is_average_step_supported(step))
        return -1;

    // currently we are using m (minute) h (hour), d (day)
    int c          = step[strlen(step) - 1];
    int multiplier = -1;
    switch (c) {
        case 109: // minute
        {
            multiplier = 60;
            break;
        }
        case 104: // hour
        {
            multiplier = 3600;
            break;
        }
        case 100: // day
        {
            multiplier = 86400;
            break;
        }
        default:
            return -1;
    }
    char* substr = strndup(step, strlen(step) - 1);
    if (!substr)
        return -1;
    int number = atoi(substr);
    free(substr);
    substr = NULL;
    return int64_t(number * multiplier);
}

int64_t datetime_to_calendar(const char* datetime)
{
    if (!datetime || strlen(datetime) != DATETIME_FORMAT_LENGTH)
        return -1;
    int  year=0, month=0, day=0, hour=0, minute=0, second=0;
    char suffix=0;
    int  rv = sscanf(datetime, DATETIME_FORMAT, &year, &month, &day, &hour, &minute, &second, &suffix);
    if (rv != 7 || suffix != 'Z') {
        return -2;
    }
    struct tm tm;
    tm.tm_year = year - 1900;
    tm.tm_mon  = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min  = minute;
    tm.tm_sec  = second;
    tm.tm_isdst = 0;
    int64_t t  = my_timegm(&tm);
    if (t < 0) return -3;
    return t;
}


int calendar_to_datetime(time_t timestamp, char* buffer, size_t n)
{
    struct tm* tmp = gmtime(&timestamp);
    if (!tmp || strftime(buffer, n, STRFTIME_DATETIME_FORMAT, tmp) ==
                    0) { // it's safe to check for 0, since we expect non-zero string
        return -1;
    }
    return 0;
}

int64_t my_timegm(struct tm* tm)
{
    std::lock_guard<std::mutex> lock (timegm_mux);

    // set the TZ environment variable to UTC, call mktime(3) and restore the value of TZ.
    char*  tz = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();
    time_t ret = mktime(tm);
    if (tz)
        setenv("TZ", tz, 1);
    else
        unsetenv("TZ");
    tzset();
    if (tm->tm_isdst != 0)
        return -1;
    return ret;
}

int32_t string_to_int32(const char* value)
{
    char*   end = NULL;
    int32_t result = 0;
    errno = 0;
    if (!value) {
        errno = EINVAL;
        return INT32_MAX;
    }
    result = int32_t(strtol(value, &end, 10));
    if (*end)
        errno = EINVAL;
    if (errno)
        return INT32_MAX;
    return result;
}

uint32_t string_to_uint32(const char* value)
{
    char*    end = NULL;
    uint32_t result = 0;
    errno = 0;
    if (!value) {
        errno = EINVAL;
        return UINT32_MAX;
    }
    result = uint32_t(strtoul(value, &end, 10));
    if (*end)
        errno = EINVAL;
    if (errno) {
        return UINT32_MAX;
    }
    return result;
}

int16_t string_to_int16(const char* value)
{
    int32_t result = string_to_int32(value);
    if (result > INT16_MAX || result < INT16_MIN) {
        errno = EINVAL;
        return INT16_MAX;
    }
    return int16_t(result);
}

uint16_t string_to_uint16(const char* value)
{
    uint32_t result = string_to_uint32(value);
    if (result > UINT16_MAX) {
        errno = EINVAL;
        return UINT16_MAX;
    }
    return uint16_t(result);
}

int8_t string_to_int8(const char* value)
{
    int32_t result = string_to_int32(value);
    if (result > INT8_MAX || result < INT8_MIN) {
        errno = EINVAL;
        return INT8_MAX;
    }
    return int8_t(result);
}

uint8_t string_to_uint8(const char* value)
{
    uint32_t result = string_to_uint32(value);
    if (result > UINT8_MAX) {
        errno = EINVAL;
        return UINT8_MAX;
    }
    return uint8_t(result);
}

double string_to_double(const char* value)
{
    char*  end = NULL;
    double result = strtod(value, &end);
    if (*end)
        errno = EINVAL;
    if (errno)
        return std::nan("");
    return result;
}
