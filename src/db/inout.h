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

/// \file   inout.h
/// \brief  Import/Export of csv
/// \author Michal Vyskocil <MichalVyskocil@Eaton.com>
/// \author Alena Chernikava <AlenaChernikava@Eaton.com>
#pragma once

#include <iostream>
#include <string>
#include <set>

#define CREATE_MODE_ONE_ASSET 1
#define CREATE_MODE_CSV       2

namespace persist {

/// export csv file and write result to output stream
///
/// @param[out] out - a reference to the standard output stream to which content will be written
/// @param[in] dc_id - limit export to this DC id (default -1 means all DCs)
/// @param[in] generate_bom - generate BOM or not (default true)
void export_asset_csv(std::ostream& out, int64_t dc_id = -1, bool generate_bom = true);

void export_asset_json(std::ostream& out, std::set<std::string>* listElement = NULL);

} // namespace persist
