/*
 *
 * Copyright (C) 2015 - 2020 Eaton
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

#include "shared/utilspp.h"

#include <sstream>
#include <limits>
#include <cmath>
#include <algorithm>
#include <fty_log.h>
#include <fty_common.h>

// Initialize the logger for tntnet process
static int setFtylog()
{
    ManageFtyLog::setInstanceFtylog("tntnet", "/etc/fty/wwwlog.cfg");
    return 1;
}

static int _ftylog = setFtylog();

namespace utils {

std::string strip (const std::string& strIn)
{
    std::string str{strIn};
    str.erase(remove_if(str.begin(), str.end(), isspace), str.end());
    return str;
}

std::string join (const char **str_arr, size_t length, const char *separator)
{
    std::string result;

    if (str_arr && (*str_arr) && separator) {
        result.append (str_arr[0]);

        size_t i = 1;
        while ((i < length) && str_arr[i]) {
            result.append (separator).append (str_arr[i]);
            ++i;
        }
    }

    return result;
}

std::string join (const char **str_arr, const char *separator)
{
    return join(str_arr, ((str_arr && (*str_arr)) ? strlen(*str_arr) : 0), separator);
}

} // namespace utils

