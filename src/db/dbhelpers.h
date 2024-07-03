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

/// @file   dbhelpers.h
/// @brief  Helper function for direct interact with DB
/// @author Alena Chernikava <AlenaChernikava@Eaton.com>
#pragma once

#include <string>
#include <tuple>

/// A type for storing basic information about device.
///
/// First  -- id, asset element id of the device in database.
/// Second -- device_name, asset element name of the device in database.
/// Third  -- device_type_name, name of the device type in database.
/// Forth  -- device_type_id, id of the device type in database.

using device_info_t = std::tuple<uint32_t, std::string, std::string, uint32_t>;

inline uint32_t device_info_id(const device_info_t& d)
{
    return std::get<0>(d);
}

inline uint32_t device_info_type_id(const device_info_t& d)
{
    return std::get<3>(d);
}

inline std::string device_info_name(const device_info_t& d)
{
    return std::get<1>(d);
}

inline std::string device_info_type_name(const device_info_t& d)
{
    return std::get<2>(d);
}

