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
#include "cleanup.h"
#include <assert.h>
#include <fty_common.h>
#include <mutex>

std::mutex timegm_mux;  // Mutex for my_timegm function which is not thread-safe

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
    _scoped_char* substr = strndup(step, strlen(step) - 1);
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
    time_t ret;
    char*  tz;
    tz = getenv("TZ");
    setenv("TZ", "", 1);
    tzset();
    ret = mktime(tm);
    if (tz)
        setenv("TZ", tz, 1);
    else
        unsetenv("TZ");
    tzset();
    if (tm->tm_isdst != 0)
        return -1;
    return ret;
}

int64_t string_to_int64(const char* value)
{
    char*   end;
    int64_t result;
    errno = 0;
    if (!value) {
        errno = EINVAL;
        return INT64_MAX;
    }
    result = strtoll(value, &end, 10);
    if (*end)
        errno = EINVAL;
    if (errno)
        return INT64_MAX;
    return result;
}

int32_t string_to_int32(const char* value)
{
    char*   end;
    int32_t result;
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

uint64_t string_to_uint64(const char* value)
{
    char*    end;
    uint64_t result;
    errno = 0;
    if (!value) {
        errno = EINVAL;
        return UINT64_MAX;
    }
    result = strtoull(value, &end, 10);
    if (*end)
        errno = EINVAL;
    if (errno)
        return UINT64_MAX;
    return result;
}

uint32_t string_to_uint32(const char* value)
{
    char*    end;
    uint32_t result;
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
    char*  end;
    double result = strtod(value, &end);
    if (*end)
        errno = EINVAL;
    if (errno)
        return std::nan("");
    return result;
}

bool addi32_overflow(int32_t a, int32_t b, int32_t* res)
{
    int64_t l_res = a + b;
    if (l_res > INT32_MAX || l_res < INT32_MIN)
        return false;
    *res = int32_t(l_res); // this is safe because of check above
    return true;
}

bool bsi32_rescale(int32_t in_value, int8_t in_scale, int8_t new_scale, int32_t* value)
{
    if (in_scale == new_scale) {
        *value = in_value;
        return true;
    }

    int32_t l_value = in_value;

    if (in_scale > new_scale) {
        for (int i = 0; i != abs(in_scale - new_scale); i++) {
            if (l_value >= (INT32_MAX / 10) || l_value <= (INT32_MIN / 10))
                return false;
            l_value *= 10;
        }
        *value = l_value;
        return true;
    }

    for (int i = 0; i != abs(in_scale - new_scale); i++) {
        l_value /= 10;
    }
    *value = l_value;
    return true;
}

bool bsi32_add(int32_t value1, int8_t scale1, int32_t value2, int8_t scale2, int32_t* value, int8_t* scale)
{
    bool ret = false;

    int32_t l_value = 0, l_value1 = 0, l_value2 = 0;
    int8_t  l_scale = 0;

    l_scale = (scale1 < scale2) ? scale1 : scale2;

    ret = bsi32_rescale(value1, scale1, l_scale, &l_value1);
    if (!ret)
        return false;

    ret = bsi32_rescale(value2, scale2, l_scale, &l_value2);
    if (!ret)
        return false;

    ret = addi32_overflow(l_value1, l_value2, &l_value);
    if (!ret)
        return false;

    *value = l_value;
    *scale = l_scale;
    return true;
}

#define START_IDX   0
#define ISO_IDX     1
#define EXCELL_IDX  2
#define NO_DASH_IDX 3

static const char* FORMATS[] = {"%d-%m-%Y", "%Y-%m-%d", "%d-%b-%y", "%d.%m.%Y", "%d %m %Y", "%m/%d/%Y", NULL};

// this function analyze input date to distinguish
// 1) no -, return NO_DASH_IDX
// 2) 4 characters before dash, return ISO IDX
// 3) non digit in between, return EXCELL_IDX
// 4) last 4 characters, return START_IDX for mm-dd-YYYY
// 5) return -1 otherwise
static ssize_t starting_idx(const char* inp)
{
    if (!strchr(inp, '-'))
        return NO_DASH_IDX;

    if (strchr(inp, '-') - inp == 4)
        return ISO_IDX;

    inp = strchr(inp, '-') + 1;
    if (!inp)
        return -1;

    if (!isdigit(inp[0]))
        return EXCELL_IDX;

    inp = strchr(inp, '-') + 1;
    if (!inp)
        return -1;
    if (strlen(inp) == 4)
        return START_IDX;

    return -1;
}

char* sanitize_date(const char* inp)
{
    assert(inp);

    ssize_t start = starting_idx(inp);
    if (start <= -1)
        return NULL;

    for (size_t i = size_t(start); FORMATS[i] != NULL; i++) {
        struct tm tm;
        char*     r = strptime(inp, FORMATS[i], &tm);

        if (!r)
            continue;

        char* buf = static_cast<char*>(calloc(1, 11)); // buffer for ISO date
        strftime(buf, 11, FORMATS[ISO_IDX], &tm);
        return buf;
    }

    return NULL;
}

#undef START_IDX
#undef ISO_IDX
#undef EXCELL_IDX
#undef NO_DASH_IDX
