/*
Copyright (C) 2014 - 2020 Eaton

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/// @file   utils.h
/// @brief  various random C and project wide helpers
/// @author Michal Vyskocil <MichalVyskocil@Eaton.com>
/// @author Karol Hrdina <KarolHrdina@Eaton.com>

#pragma once
#include <time.h>
#include <stdint.h>
#include <string.h>

/// Return true if string representation of average step is supported
bool is_average_step_supported(const char* step);

/// Return true if string representation of average type is supported
bool is_average_type_supported(const char* type);

/// Supported average step expressed in seconds OR -1
int64_t average_step_seconds(const char* step);

/// transform rfc-11 defined datetime format to calendar time (unix timestamp).
///
/// For $BIOS project rest api rfc-11 defines one common format for datetimes. It is "derived" from iso8601 in the
/// following sense: all hyphen (-) colon (:) characters are removed and 'Z' character is always appended to the end
/// that represents UTC timezone. Since the standard understands the two following dates to be equal "19991231235959Z"
/// == "20000101000000Z" we can not use  strptime() function as it fails on hh:mm:ss 24:00:00. Another needed workaround
/// is the fact that mktime() uses local time zone.
/// @return unix timestamp or -1 on failure
int64_t datetime_to_calendar(const char* datetime);

/// convert unix time to rest api time
///
/// @param t - unix time
/// @param buf - caller's buffer large enough to store the string
/// @param s - size of the buffer
/// @return -1 in case of failure, 0 otherwise
int calendar_to_datetime(time_t timestamp, char* buffer, size_t n);

/// portable version of timegm() taken from manual pages
int64_t my_timegm(struct tm* tm);

/// converts char* to int32_t
///
/// In case of error INT32_MAX is returned and errno is set.
///
/// @param value string (char *), containing the number
/// @return text value converted to int32_t
int32_t string_to_int32(const char* value);

/// converts char* to uint32_t
///
/// In case of error UINT32_MAX is returned and errno is set.
///
/// @param value string (char *), containing the number
/// @return text value converted to uint32_t
uint32_t string_to_uint32(const char* value);

int16_t  string_to_int16(const char* value);
uint16_t string_to_uint16(const char* value);

int8_t  string_to_int8(const char* value);
uint8_t string_to_uint8(const char* value);

double string_to_double(const char* value);
