/*
Copyright (C) 2015 - 2020 Eaton

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

/// @file utilspp.h
/// @brief c++ utilities
/// @author Karol Hrdina <KarolHrdina@Eaton.com>

#pragma once

#include <sstream>
#include <map>
#include <string>
#include <cstdint>

namespace utils {

/// strip whitespaces from input string
/// @param[in] _str is a string to strip
/// @return new allocated string with whitespaces deleted
std::string strip(const std::string& str);

/// Join keys of std::map using given separator
///
/// Applicable only to maps that have key (i.e. first templated parameter) convertible to std::string or basic
/// arithmetic type
///
/// @param[in] m - the std::map
/// @param[in] separator - the separator
template <typename K, typename V>
std::string join_keys_map(const std::map<K, V>& t, const std::string& separator)
{
    static_assert(
        std::is_convertible<K, std::string>::value || std::is_arithmetic<K>::value,
        "Must be convertible to string or arithmetic type.");
    std::ostringstream tmp;
    auto               it = t.cbegin();
    for (; it != --t.cend(); ++it) {
        tmp << it->first << separator;
    }
    tmp << it->first;
    std::string result = tmp.str();
    return result;
}

/// Concatenate items of array of c strings using given separator

/// NULL encountered sooner than length items terminates the concatenation.
/// @return Concatenated string or empty on error
std::string join(const char** str_arr, size_t length, const char* separator);

/// Version of join(const char **str_arr, uint32_t length, const char *separator)" that works until NULL terminating
/// item is encountered
/// @note Use this version only for arrays that are NULL terminated!
std::string join(const char** str_arr, const char* separator);

} // namespace utils
