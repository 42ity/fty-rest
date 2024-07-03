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

/// @file upsstatus.h
/// @brief  functions to work with ups status string representation as it is used in networkupstools
/// @author Tomas Halman <TomasHalman@Eaton.com>

#pragma once

#include <cstdint>
#include <string>

namespace shared {

/// converts status from uint16_t bitmap to text representation
/// @param status uint16_t bitmap representing UPS status (ups.status bitsfield metric)
/// @return std::string text representation (for example "OL CHRG")
std::string upsstatus_to_string(uint16_t status);

} // namespace shared
