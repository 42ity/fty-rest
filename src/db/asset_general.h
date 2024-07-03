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

/*! \file   asset_general.h
    \brief  Header file for all not low level functions for asset
    \author Alena Chernikava <AlenaChernikava@eaton.com>
*/

#pragma once

#include "dbtypes.h"
#include <fty_common_db_defs.h>
#include <tntdb.h>
#include <tuple>

#define RC0_INAME "rackcontroller-0"


/// A type for storing basic information about device.
///
/// First  -- id, asset element id of the device in database.
/// Second -- device_name, asset element name of the device in database.
/// Third  -- device_type_name, name of the device type in database.
/// Forth  -- device_type_id, id of the device type in database.

using device_info_t = std::tuple<uint32_t, std::string, std::string, uint32_t>;

inline uint32_t    device_info_id(const device_info_t& d)        { return std::get<0>(d); }
inline std::string device_info_name(const device_info_t& d)      { return std::get<1>(d); }
inline std::string device_info_type_name(const device_info_t& d) { return std::get<2>(d); }
inline uint32_t    device_info_type_id(const device_info_t& d)   { return std::get<3>(d); }


namespace persist {

int update_dc_room_row_rack_group(
    tntdb::Connection&            conn,
    a_elmnt_id_t                  element_id,
    const char*                   element_name,
    a_elmnt_tp_id_t               element_type_id,
    a_elmnt_id_t                  parent_id,
    zhash_t*                      extattributes,
    const char*                   status,
    a_elmnt_pr_t                  priority,
    std::set<a_elmnt_id_t> const& groups,
    const std::string&            asset_tag,
    std::string&                  errmsg,
    zhash_t*                      extattributesRO = NULL);


int update_device(
    tntdb::Connection&            conn,
    tntdb::Transaction&           trans,
    a_elmnt_id_t                  element_id,
    const char*                   element_name,
    a_elmnt_tp_id_t               element_type_id,
    a_elmnt_id_t                  parent_id,
    zhash_t*                      extattributes,
    const char*                   status,
    a_elmnt_pr_t                  priority,
    std::set<a_elmnt_id_t> const& groups,
    std::vector<link_t>&          links,
    const std::string&            asset_tag,
    std::string&                  errmsg,
    zhash_t*                      extattributesRO = NULL);


db_reply_t insert_dc_room_row_rack_group(
    tntdb::Connection&            conn,
    const char*                   element_name,
    a_elmnt_tp_id_t               element_type_id,
    a_elmnt_id_t                  parent_id,
    zhash_t*                      extattributes,
    const char*                   status,
    a_elmnt_pr_t                  priority,
    std::set<a_elmnt_id_t> const& groups,
    const std::string&            asset_tag,
    zhash_t*                      extattributesRO = NULL);


db_reply_t insert_device(
    tntdb::Connection&            conn,
    tntdb::Transaction&           trans,
    std::vector<link_t>&          links,
    std::set<a_elmnt_id_t> const& groups,
    const char*                   element_name,
    a_elmnt_id_t                  parent_id,
    zhash_t*                      extattributes,
    a_dvc_tp_id_t                 asset_device_type_id,
    const char*                   asset_device_type_name,
    const char*                   status,
    a_elmnt_pr_t                  priority,
    const std::string&            asset_tag,
    zhash_t*                      extattributesRO = NULL);


db_reply_t delete_dc_room_row_rack(tntdb::Connection& conn, a_elmnt_id_t element_id);


db_reply_t delete_group(tntdb::Connection& conn, a_elmnt_id_t element_id);


db_reply_t delete_device(tntdb::Connection& conn, a_elmnt_id_t element_id, const std::string& asset_json = "");


} // namespace persist
