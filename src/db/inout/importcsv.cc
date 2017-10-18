/*
Copyright (C) 2015 Eaton

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

/*! \file  importcsv.cc
    \brief Implementation of csv import
    \author Michal Vyskocil <MichalVyskocil@Eaton.com>
    \author Alena Chernikava <AlenaChernikava@Eaton.com>
*/

#include <string>
#include <algorithm>
#include <ctype.h>

#include <tntdb/connect.h>
#include <cxxtools/regex.h>
#include <cxxtools/join.h>

#include "db/inout.h"

#include "log.h"
#include "assetcrud.h"
#include "dbpath.h"
#include "cleanup.h"
#include "db/asset_general.h"
#include "db/assets.h"
#include "db/inout.h"
#include "utils.h"
#include "utils++.h"
#include "utils_web.h"
#include "tntmlm.h"

using namespace shared;

namespace persist {
int
    get_priority
        (const std::string& s)
{
    if ( s.size() > 2 )
        return 5;
    for (int i = 0; i != 2; i++) {
        if (s[i] >= 49 && s[i] <= 53) {
            return s[i] - 48;
        }
    }
    return 5;
}

static std::map<std::string,int>
    read_element_types
        (tntdb::Connection &conn)
{
    auto res = get_dictionary_element_type(conn);
    // in case of any error, it would be empty
    return res.item;
}

static std::map<std::string,int>
    read_device_types
        (tntdb::Connection &conn)
{
    auto res = get_dictionary_device_type(conn);
    // in case of any error, it would be empty
    return res.item;
}

static bool
    check_u_size
        (std::string &s)
{
    static cxxtools::Regex regex("^[0-9]{1,2}[uU]?$");
    if ( !regex.match(s) )
    {
        return false;
    }
    else
    {
        // need to drop the "U"
        if ( ! (::isdigit(s.back())) )
        {
            s.pop_back();
        }
        // remove trailing 0
        if (s.size() == 2 && s[0] == '0') {
            s.erase(s.begin());
        }
        return true;
    }
}

static bool
    match_ext_attr
        (std::string &value, const std::string &key)
{
    if ( key == "u_size" )
    {
        return check_u_size(value);
    }
    return ( !value.empty() );
}

// check if key contains date
// ... warranty_end or ends with _date
static bool
    is_date (const std::string &key)
{
    static cxxtools::Regex regex{"(^warranty_end$|_date$)"};
    return regex.match (key);
}

static double
sanitize_value_double(
    const std::string &key,
    const std::string &value)
{
    try {
        std::size_t pos = 0;
        double d_value = std::stod (value, &pos);
        if  ( pos != value.length() ) {
            log_info ("Extattribute: %s='%s' is not double", key.c_str(), value.c_str());
            bios_throw ("request-param-bad", key.c_str(), ("'" + value + "'").c_str(), "value should be a number");
        }
        return d_value;
    }
    catch (const std::exception &e ) {
        log_info ("Extattribute: %s='%s' is not double", key.c_str(), value.c_str());
        bios_throw ("request-param-bad", key.c_str(), ("'" + value + "'").c_str(), "value should be a number");
    }
}


/*
 * \brief Identify id of row with rackcontroller-0
 * \return SIZE_MAX when RC-0 not found
 * \return size_t number of RC-0 row
 * \throw runtime_error on failure
 */
size_t
promote_rc0(
        MlmClient &client,
         const CsvMap& cm,
         touch_cb_t touch_fn) {
    touch_fn(); // renew request watchdog timer
    // first find all rows with rackcontrollers
    std::vector<size_t> rc_rows;
    for (size_t row_i = 1; row_i != cm.rows(); row_i++) {
        auto iname = cm.get(row_i, "id");
        if (iname.compare(0, strlen("rackcontroller-"), "rackcontroller-")) {
            rc_rows.push_back(row_i);
        }
    }
    if (0 == rc_rows.size()) {
        return SIZE_MAX;
    }
    touch_fn(); // renew request watchdog timer

    // ask for data about myself
    zuuid_t *uuid = zuuid_new ();
    zmsg_t *msg = zmsg_new ();
    zmsg_addstr (msg, "INFO");
    zmsg_addstr (msg, zuuid_str_canonical (uuid));
    if (-1 == client.sendto ("fty-info", "info", 1000, &msg)) {
        zuuid_destroy (&uuid);
        throw std::runtime_error("Sending INFO message to fty-info failed");
    }
    touch_fn(); // renew request watchdog timer
    // parse receieved data
    zmsg_t *resp = client.recv (zuuid_str_canonical (uuid), 5);
    touch_fn(); // renew request watchdog timer
    if (NULL == resp) {
        zuuid_destroy (&uuid);
        throw std::runtime_error("Response on INFO message from fty-info is empty");
    }
    char *command = zmsg_popstr (resp);
    if (0 == strcmp("ERROR", command)) {
        zuuid_destroy (&uuid);
        throw std::runtime_error("Response on INFO message from fty-info is ERROR");
    }
    char *srv_name  = zmsg_popstr (resp); // we don't really need those, but we need to popstr them
    char *srv_type  = zmsg_popstr (resp);
    char *srv_stype = zmsg_popstr (resp);
    char *srv_port  = zmsg_popstr (resp);
    zframe_t *frame_infos = zmsg_next (resp);
    zhash_t *info = zhash_unpack(frame_infos); // serial, hostname, ip.[1-3]
    zstr_free (&command);
    zstr_free(&srv_name);
    zstr_free(&srv_type);
    zstr_free(&srv_stype);
    zstr_free(&srv_port);
    //zhash_destroy(&info);
    zmsg_destroy(&resp);
    char *ip1 = (char *)zhash_lookup(info, "ip.1");
    char *ip2 = (char *)zhash_lookup(info, "ip.2");
    char *ip3 = (char *)zhash_lookup(info, "ip.3");
    char *serial = (char *)zhash_lookup(info, "serial");
    char *hostname = (char *)zhash_lookup(info, "hostname");
    zmsg_destroy (&resp);

    touch_fn(); // renew request watchdog timer
    // get list of rackcontrollers already in database
    msg = zmsg_new ();
    zmsg_addstr (msg, "GET");
    zmsg_addstr (msg, "rackcontroller");
    int rv = client.sendto ("asset-agent", "ASSETS", 5000, &msg);
    touch_fn(); // renew request watchdog timer
    if (rv != 0) {
        zhash_destroy(&info);
        throw std::runtime_error("Request for rackcontroller list failed");
    }
    resp = client.recv (zuuid_str_canonical (uuid), 5);
    touch_fn(); // renew request watchdog timer
    command = zmsg_popstr (resp);
    if (0 == strcmp("ERROR", command)) {
        zuuid_destroy (&uuid);
        throw std::runtime_error("Response for rackcontroller from fty-asset is ERROR");
    }
    zstr_free (&command);
    char *asset = zmsg_popstr(resp);
    std::vector<char *> rackcontrollers;
    while (asset) {
        rackcontrollers.push_back(asset);
        asset = zmsg_popstr(resp);
    }
    zmsg_destroy (&resp);

    touch_fn(); // renew request watchdog timer
    // check how many RCs we already have in database
    if (0 == rackcontrollers.size()) {
        // as discussed with Arno, there are no options, one must be picked
        // nothing to do, promote first RC suitable to RC-0
        if (1 == rc_rows.size()) {
            zhash_destroy(&info);
            return rc_rows[0];
        }
        // check for rackcontroller-0 first
        for (size_t row_i : rc_rows) {
            auto iname = cm.get(row_i, "id");
            if ("rackcontroller-0" == iname) {
                zhash_destroy(&info);
                return row_i;
            }
        }
        // then check ip address
        for (size_t row_i : rc_rows) {
            auto ip = cm.get(row_i, "ip.1");
            if (ip == ip1 || ip == ip2 || ip == ip3) {
                zhash_destroy(&info);
                return row_i;
            }
        }
        // sadly uuid isn't part of import csv
        // then check serial number
        for (size_t row_i : rc_rows) {
            auto s_no = cm.get(row_i, "serial_no");
            if (serial == s_no ) {
                zhash_destroy(&info);
                return row_i;
            }
        }
        // then check hostname
        for (size_t row_i : rc_rows) {
            auto hname = cm.get(row_i, "hostname.1");
            if (hostname == hname ) {
                zhash_destroy(&info);
                return row_i;
            }
        }
        // then check hostname
        for (size_t row_i : rc_rows) {
            auto hname = cm.get(row_i, "hostname.1");
            if (hostname == hname ) {
                zhash_destroy(&info);
                return row_i;
            }
        }
        // we're out of options, take first available
        zhash_destroy(&info);
        return rc_rows[0];
    }
    else {
        // as discussed with Arno, if IP match my IP, this is probably replacement, otherwise it's import
        for (size_t row_i : rc_rows) {
            auto ip = cm.get(row_i, "ip.1");
            if (ip == ip1 || ip == ip2 || ip == ip3) {
                zhash_destroy(&info);
                return row_i;
            }
        }
        for (size_t row_i : rc_rows) {
            auto iname = cm.get(row_i, "id");
            auto s_no = cm.get(row_i, "serial_no");
            if ("rackcontroller-0" == iname && serial == s_no ) {
                zhash_destroy(&info);
                return row_i;
            }
        }
        zhash_destroy(&info);
        return SIZE_MAX;
    }
}


/*
 * \brief Replace user defined names with internal names
 */
std::map <std::string, std::string>sanitize_row_ext_names (
    const CsvMap &cm,
    size_t row_i,
    bool sanitize
)
{
    std::map <std::string, std::string> result;
    // make copy of this one line
    for (auto title: cm.getTitles ()) {
        result[title] = cm.get(row_i, title);
    }
    if (sanitize) {
        // sanitize ext names to t_bios_asset_element.name
        auto sanitizeList = {"location", "logical_asset", "power_source.", "group." };
        for (auto item: sanitizeList) {
            if (item [strlen (item) - 1] == '.') {
                // iterate index .X
                for (int i = 1; true; ++i) {
                    std::string title = item + std::to_string (i);
                    auto it = result.find (title);
                    if (it == result.end ()) break;

                    std::string name;
                    int rv = extname_to_asset_name (it->second, name);
                    if (rv != 0) { name = it->second; }
                    log_debug ("sanitized %s '%s' -> '%s'", title.c_str(), it->second.c_str(), name.c_str ());
                    result [title] = name;
                }
            } else {
                // simple name
                auto it = result.find (item);
                if (it != result.end ()) {
                    std::string name;
                    int rv = extname_to_asset_name (it->second, name);
                    if (rv != 0) { name = it->second; }
                    log_debug ("sanitized %s '%s' -> '%s'", it->first.c_str (), it->second.c_str(), name.c_str ());
                    result [item] = name;
                }
            }
        }
    }
    return result;
}

/*
 * \brief Processes a single row from csv file
 *
 * \param[in] conn     - a connection to DB
 * \param[in] cm       - already parsed csv file
 * \param[in] row_i    - number of row to process
 * \param[in] TYPES    - list of available types
 * \param[in] SUBTYPES - list of available subtypes
 * \param[in][out] ids - list of already seen asset ids
 *
 */
static std::pair<db_a_elmnt_t, persist::asset_operation>
    process_row
        (tntdb::Connection &conn,
         const CsvMap &cm,
         size_t row_i,
         const std::map<std::string,int> &TYPES,
         const std::map<std::string,int> &SUBTYPES,
         std::set<a_elmnt_id_t> &ids,
         bool sanitize,
         size_t rc_0
         )
{
    LOG_START;

    log_debug ("################ Row number is %zu", row_i);
    static const std::set<std::string> STATUSES = \
        {"active", "nonactive", "spare", "retired"};

    // get location, powersource etc as name from ext.name
    auto sanitizedAssetNames = sanitize_row_ext_names (cm, row_i, sanitize);

    // This is used to track, which columns had been already processed,
    // because if they was't processed yet,
    // then they should be treated as external attributes
    auto unused_columns = cm.getTitles();

    if (unused_columns.empty()) {
        bios_throw("bad-request-document", "Cannot import empty document.");
    }

    // because id is definitely not an external attribute
    auto id_str = unused_columns.count("id") ? cm.get(row_i, "id") : "";
    if ("rackcontroller-0" == id_str && rc_0 == SIZE_MAX ) {
        // we got RC-0 but it don't match "myself", change it to something else ("")
        id_str = "";
    } else if (rc_0 == row_i && id_str != "rackcontroller-0") {
        id_str = "rackcontroller-0";
    }

    unused_columns.erase("id");
    persist::asset_operation operation = persist::asset_operation::INSERT;
    int64_t id = 0;
    if ( !id_str.empty() )
    {
        id = name_to_asset_id (id_str);
        if (id == -1) {
            bios_throw("element-not-found", id_str.c_str ());
        }
        if ( ids.count(id) == 1 ) {
            std::string msg = "Element id '";
            msg += id_str;
            msg += "' found twice, aborting";
            bios_throw("bad-request-document", msg.c_str());
        }
        ids.insert(id);
        operation = persist::asset_operation::UPDATE;
    }

    auto ename = cm.get(row_i, "name");
    if (ename.empty ())
        bios_throw("request-param-bad", "name", "<empty>", "<unique, non empty value>");
    std::string iname;
    int rv = extname_to_asset_name (ename, iname);
    log_debug ("name = '%s/%s'", ename.c_str(), iname.c_str());
    if (rv == -2) {
        bios_throw("internal-error", "Database failure");
    }
    unused_columns.erase("name");

    auto type = cm.get_strip(row_i, "type");
    log_debug ("type = '%s'", type.c_str());
    if ( TYPES.find(type) == TYPES.end() ) {
        bios_throw("request-param-bad", "type", type.empty() ? "<empty>" : type.c_str(), utils::join_keys_map(TYPES, ", ").c_str());
    }
    auto type_id = TYPES.find(type)->second;
    unused_columns.erase("type");

    auto status = cm.get_strip(row_i, "status");
    log_debug ("status = '%s'", status.c_str());
    if ( STATUSES.find(status) == STATUSES.end() ) {
        bios_throw ("request-param-bad", "status", status.empty() ? "<empty>" : status.c_str(),
            cxxtools::join(STATUSES.cbegin(), STATUSES.cend(), ", ").c_str());
    }
    unused_columns.erase("status");

    auto asset_tag =  unused_columns.count("asset_tag") ? cm.get(row_i, "asset_tag") : "";
    log_debug ("asset_tag = '%s'", asset_tag.c_str());
    if ( ( !asset_tag.empty() ) && ( asset_tag.length() > 50 ) ){
        bios_throw("request-param-bad", "asset_tag", "<too long>", "<unique string from 1 to 50 characters>");
    }
    unused_columns.erase("asset_tag");

    int priority = get_priority(cm.get_strip(row_i, "priority"));
    log_debug ("priority = %d", priority);
    unused_columns.erase("priority");

    auto location = sanitizedAssetNames ["location"];
    log_debug ("location = '%s'", location.c_str());
    a_elmnt_id_t parent_id = 0;
    if ( !location.empty() )
    {
        auto ret = select_asset_element_by_name(conn, location.c_str());
        if ( ret.status == 1 )
            parent_id = ret.item.id;
        else {
            if (ret.errsubtype == DB_ERROR_NOTFOUND) {
                bios_throw("request-param-bad", "location", location.c_str(), "<existing asset name>");
            }
            else {
                bios_throw("internal-error", "Database failure");
            }
        }
    }
    unused_columns.erase("location");

    // Business requirement: be able to write 'rack controller', 'RC', 'rc' as subtype == 'rack controller'
    std::map<std::string,int> local_SUBTYPES = SUBTYPES;
    int rack_controller_id = SUBTYPES.find ("rack controller")->second;
    int patch_panel_id = SUBTYPES.find ("patch panel")->second;

    local_SUBTYPES.emplace (std::make_pair ("rackcontroller", rack_controller_id));
    local_SUBTYPES.emplace (std::make_pair ("rackcontroler", rack_controller_id));
    local_SUBTYPES.emplace (std::make_pair ("rc", rack_controller_id));
    local_SUBTYPES.emplace (std::make_pair ("RC", rack_controller_id));
    local_SUBTYPES.emplace (std::make_pair ("RC3", rack_controller_id));

    local_SUBTYPES.emplace (std::make_pair ("patchpanel", patch_panel_id));

    auto subtype = cm.get_strip (row_i, "sub_type");

    log_debug ("subtype = '%s'", subtype.c_str());
    if ( ( type == "device" ) &&
         ( local_SUBTYPES.find(subtype) == local_SUBTYPES.cend() ) ) {
        bios_throw("request-param-bad", "subtype", subtype.empty() ? "<empty>" : subtype.c_str(), utils::join_keys_map(SUBTYPES, ", ").c_str());
    }

    if ( ( !subtype.empty() ) && ( type != "device" ) && ( type != "group") )
    {
        log_warning ("'%s' - subtype is ignored", subtype.c_str());
    }

    if ( ( subtype.empty() ) && ( type == "group" ) ) {
        bios_throw("request-param-required", "subtype (for type group)");
    }

    auto subtype_id = local_SUBTYPES.find(subtype)->second;
    unused_columns.erase("sub_type");

    // now we have read all basic information about element
    // if id is set, then it is right time to check what is going on in DB
    if ( !id_str.empty() )
    {
        db_reply <db_web_basic_element_t> element_in_db = select_asset_element_web_byId
                                                        (conn, id);
        if ( element_in_db.status == 0 ) {
            if (element_in_db.errsubtype == DB_ERROR_NOTFOUND) {
                bios_throw("element-not-found", id_str.c_str());
            }
            else {
                bios_throw("internal-error", "Database failure");
            }
        }
        else
        {
            if ( element_in_db.item.type_id != type_id ) {
                bios_throw("bad-request-document", "Changing of asset type is forbidden");
            }
            if ( ( element_in_db.item.subtype_id != subtype_id ) &&
                 ( element_in_db.item.subtype_name != "N_A" ) ) {
                bios_throw("bad-request-document", "Changing of asset subtype is forbidden");
            }
        }
    }

    std::string group;

    // list of element ids of all groups, the element belongs to
    std::set <a_elmnt_id_t>  groups{};
    for ( int group_index = 1 ; true; group_index++ )
    {
        std::string grp_col_name = "";
        try {
            // column name
            grp_col_name = "group." + std::to_string(group_index);
            // remove from unused
            unused_columns.erase(grp_col_name);
            // take value
            group = sanitizedAssetNames.at (grp_col_name);
        }
        catch (const std::out_of_range &e)
        // if column doesn't exist, then break the cycle
        {
            log_debug ("end of group processing");
            log_debug ("%s", e.what());
            break;
        }
        log_debug ("group_name = '%s'", group.c_str());
        // if group was not specified, just skip it
        if ( !group.empty() )
        {
            // find an id from DB
            auto ret = select_asset_element_by_name(conn, group.c_str());
            if ( ret.status == 1 )
                groups.insert(ret.item.id);  // if OK, then take ID
            else
            {
                if (ret.errsubtype == DB_ERROR_NOTFOUND) {
                    log_error ("group '%s' is not present in DB, rejected", group.c_str());
                    bios_throw("element-not-found", group.c_str());
                }
                else {
                    bios_throw("internal-error", "Database failure");
                }
            }
        }
    }

    std::vector <link_t>  links{};
    std::string  link_source = "";
    for ( int link_index = 1; true; link_index++ )
    {
        link_t one_link{0, 0, NULL, NULL, 0};
        std::string link_col_name = "";
        try {
            // column name
            link_col_name = "power_source." +
                                                std::to_string(link_index);
            // remove from unused
            unused_columns.erase(link_col_name);
            // take value
            link_source = sanitizedAssetNames.at (link_col_name);
        }
        catch (const std::out_of_range &e)
        // if column doesn't exist, then break the cycle
        {
            log_debug ("end of power links processing");
            log_debug ("%s", e.what());
            break;
        }

        log_debug ("power_source_name = '%s'", link_source.c_str());
        if ( !link_source.empty() ) // if power source is not specified
        {
            // find an id from DB
            auto ret = select_asset_element_by_name
                (conn, link_source.c_str());
            if ( ret.status == 1 )
                one_link.src = ret.item.id;  // if OK, then take ID
            else
            {
                if (ret.errsubtype == DB_ERROR_NOTFOUND) {
                    log_warning ("power source '%s' is not present in DB, rejected",
                    link_source.c_str());
                    bios_throw("element-not-found", link_source.c_str());
                }
                else {
                    bios_throw("internal-error", "Database failure");
                }
            }
        }

        // column name
        auto link_col_name1 = "power_plug_src." + std::to_string(link_index);
        try{
            // remove from unused
            unused_columns.erase(link_col_name1);
            // take value
            auto link_source1 = cm.get(row_i, link_col_name1);
            // TODO: bad idea, char = byte
            // FIXME: THIS IS MEMORY LEAK!!!
            one_link.src_out = new char [4];
            strcpy ( one_link.src_out, link_source1.substr (0,4).c_str());
        }
        catch (const std::out_of_range &e)
        {
            log_debug ("'%s' - is missing at all", link_col_name1.c_str());
            log_debug ("%s", e.what());
        }

        // column name
        auto link_col_name2 = "power_input." + std::to_string(link_index);
        try{
            unused_columns.erase(link_col_name2); // remove from unused
            auto link_source2 = cm.get(row_i, link_col_name2);// take value
            // TODO: bad idea, char = byte
            // FIXME: THIS IS MEMORY LEAK!!!
            one_link.dest_in = new char [4];
            strcpy ( one_link.dest_in, link_source2.substr (0,4).c_str());
        }
        catch (const std::out_of_range &e)
        {
            log_debug ("'%s' - is missing at all", link_col_name2.c_str());
            log_debug ("%s", e.what());
        }

        if ( one_link.src != 0 ) // if first column was ok
        {
            if ( type == "device" )
            {
                one_link.type = 1; // TODO remove hardcoded constant
                links.push_back(one_link);
            }
            else
            {
                log_warning ("information about power sources is ignored for type '%s'", type.c_str());
            }
        }
    }
    _scoped_zhash_t *extattributes = zhash_new();
    zhash_autofree(extattributes);
    zhash_insert (extattributes, "name", (void *) ename.c_str ());
    for ( auto &key: unused_columns )
    {
        // try is not needed, because here are keys that are definitely there
        std::string value = cm.get(row_i, key);

        // BIOS-1564: sanitize the date for warranty_end -- start
        if (is_date (key) && !value.empty()) {
            char *date = sanitize_date (value.c_str());
            if (!date) {
                log_info ("Cannot sanitize %s '%s' for device '%s'", key.c_str(), value.c_str(), ename.c_str());
                bios_throw("request-param-bad", key.c_str(), value.c_str(), "ISO date");
            }
            value = date;
            zstr_free (&date);
        }
        // BIOS-1564 -- end

        // BIOS-2302: Check some attributes for sensors
        // BIOS-2784: Check max_current, max_power
        if ( key == "logical_asset" && !value.empty() ) {
            // check, that this asset exists

            value = sanitizedAssetNames.at ("logical_asset");

            auto ret = select_asset_element_by_name
                (conn, value.c_str());
            if ( ret.status == 0 ) {
                if (ret.errsubtype == DB_ERROR_NOTFOUND) {
                    log_info ("logical_asset '%s' does not present in DB, rejected",
                    value.c_str());
                    bios_throw("element-not-found", value.c_str());
                }
                else {
                    bios_throw("internal-error", "Database failure");
                }

                log_info ("logical_asset '%s' does not present in DB, rejected",
                    value.c_str());
                bios_throw("element-not-found", value.c_str());
            }
        }
        else
        if (    ( key == "calibration_offset_t" || key == "calibration_offset_h" )
             &&   !value.empty() )
        {
            // we want exceptions to propagate to upper layer
            sanitize_value_double (key, value);
        }
        else
        if (    ( key == "max_current" || key == "max_power" )
             &&   !value.empty() )
        {
            // we want exceptions to propagate to upper layer
            double d_value = sanitize_value_double (key, value);
            if ( d_value < 0 ) {
                log_info ("Extattribute: %s='%s' is neither positive not zero", key.c_str(), value.c_str());
                bios_throw ("request-param-bad", key.c_str(), ("'" + value + "'").c_str(), "value must be a not negative number");
            }
        }
        // BIOS-2781
        if ( key == "location_u_pos" && !value.empty() ) {
            unsigned long ul = 0;
            try {
                std::size_t pos = 0;
                ul = std::stoul (value, &pos);
                if  ( pos != value.length() ) {
                    log_info ("Extattribute: %s='%s' is not unsigned integer", key.c_str(), value.c_str());
                    bios_throw ("request-param-bad", key.c_str(), ("'" + value + "'").c_str(), "value must be an unsigned integer");
                }
            }
            catch (const std::exception& e) {
                log_info ("Extattribute: %s='%s' is not unsigned integer", key.c_str(), value.c_str());
                bios_throw ("request-param-bad", "location_u_pos", ("'" + value + "'").c_str (), "value must be an unsigned integer");
            }
            if (ul == 0 || ul > 52)
                bios_throw ("request-param-bad", "location_u_pos", ("'" + value + "'").c_str (), "value must be between <1, rack size>, where rack size must be <= 52.");
        }
        // BIOS-2799
        if ( key == "u_size" && !value.empty() ) {
            unsigned long ul = 0;
            try {
                std::size_t pos = 0;
                ul = std::stoul (value, &pos);
                if  ( pos != value.length() ) {
                    log_info ("Extattribute: %s='%s' is not unsigned integer", key.c_str(), value.c_str());
                    bios_throw ("request-param-bad", key.c_str(), ("'" + value + "'").c_str(), "value must be an unsigned integer");
                }
            }
            catch (const std::exception& e) {
                log_info ("Extattribute: %s='%s' is not unsigned integer", key.c_str(), value.c_str());
                bios_throw ("request-param-bad", "u_size", ("'" + value + "'").c_str (), "value must be an unsigned integer");
            }
            if (ul == 0 || ul > 52)
                bios_throw ("request-param-bad", "u_size", ("'" + value + "'").c_str (), "value must be between <1, rack size>, where rack size must be <= 52.");
        }

        if ( match_ext_attr (value, key) )
        {
            // ACE: temporary disabled
            // for testing purposes for rabobank usecase
            // IMHO: There is no sense to check it at all
            //   * as we cannot guarantee that manufacturers in the whole world use UNIQUE serial numbers
            //   * not unique serial number of the device should not forbid users to monitor their devices
            /*
            if ( key == "serial_no" )
            {

                if  ( unique_keytag (conn, key, value, id) == 0 )
                    zhash_insert (extattributes, key.c_str(), (void*)value.c_str());
                else
                {
                    bios_throw("request-param-bad", "serial_no", value.c_str(), "<unique string>");
                }
                continue;
            }
            */
            zhash_insert (extattributes, key.c_str(), (void*)value.c_str());
        }
    }
    // if the row represents group, the subtype represents a type
    // of the group.
    // As group has no special table as device, then this information
    // sould be inserted as external attribute
    // this parametr is mandatory according rfc-11
    if ( type == "group" )
        zhash_insert (extattributes, "type", (void*) subtype.c_str() );

    db_a_elmnt_t m;

    if ( !id_str.empty() )
    {
        _scoped_zhash_t *extattributesRO = zhash_new();
        zhash_autofree(extattributesRO);
        if(cm.getUpdateTs() != "")
            zhash_insert(extattributesRO, "update_ts", (void*) cm.getUpdateTs().c_str());
        if(cm.getUpdateUser() != "")
            zhash_insert(extattributesRO, "update_user", (void*) cm.getUpdateUser().c_str());
        m.id = id;
        std::string errmsg = "";
        if (type != "device" )
        {
            auto ret = update_dc_room_row_rack_group
                (conn, m.id, iname.c_str(), type_id, parent_id,
                 extattributes, status.c_str(), priority, groups, asset_tag, errmsg, extattributesRO);
            if ( ( ret ) || ( !errmsg.empty() ) ) {
                throw std::invalid_argument(errmsg);
            }
        }
        else
        {
            auto ret = update_device
                (conn, m.id, iname.c_str(), type_id, parent_id,
                 extattributes, status.c_str(), priority, groups, links, asset_tag, errmsg, extattributesRO);
            if ( ( ret ) || ( !errmsg.empty() ) ) {
                throw std::invalid_argument(errmsg);
            }
        }
    }
    else
    {
            _scoped_zhash_t *extattributesRO = zhash_new();
            zhash_autofree(extattributesRO);
            if(cm.getCreateMode() != 0)
                zhash_insert(extattributesRO, "create_mode", (void*) std::to_string(cm.getCreateMode()).c_str());
            if(cm.getCreateUser() != "")
                zhash_insert(extattributesRO, "create_user", (void*) cm.getCreateUser().c_str());
        if ( type != "device" )
        {
            // this is a transaction
            auto ret = insert_dc_room_row_rack_group
                (conn, ename.c_str(), type_id, parent_id,
                 extattributes, status.c_str(), priority, groups, asset_tag, extattributesRO);
            if ( ret.status != 1 ) {
                throw BiosError(ret.rowid, ret.msg);
            }
            m.id = ret.rowid;
        }
        else
        {
            // this is a transaction
            auto ret = insert_device (conn, links, groups, ename.c_str(),
                    parent_id, extattributes, subtype_id, subtype.c_str(), status.c_str(),
                    priority, asset_tag, extattributesRO);
            if ( ret.status != 1 ) {
                throw BiosError(ret.rowid, ret.msg);
            }
            m.id = ret.rowid;
        }
    }
    rv = extname_to_asset_name (ename, m.name);
    if (rv != 0)
         bios_throw("internal-error", "Database failure");
    m.status = status;
    m.parent_id = parent_id;
    m.priority = priority;
    m.type_id = type_id;
    m.subtype_id = subtype_id;
    m.asset_tag = asset_tag;

    for (void* it = zhash_first (extattributes); it != NULL;
               it = zhash_next (extattributes)) {
        m.ext.emplace (zhash_cursor (extattributes), (char*) it);
    }

    LOG_END;
    return std::make_pair(m, operation) ;
}


/*
 * \brief Checks if mandatory columns are missing in csv file
 *
 * This check is implemented according BAM DC010
 *
 * \param cm - already parsed csv file
 *
 * \return emtpy string if everything is ok, otherwise the name of missing row
 */
//MVY: moved out to support friendly error messages below
static std::vector<std::string> MANDATORY = {
    "name", "type", "sub_type", "location", "status",
    "priority"
};
static std::string
mandatory_missing
        (const CsvMap &cm)
{
    auto all_fields = cm.getTitles();
    for (const auto& s : MANDATORY) {
        if (all_fields.count(s) == 0)
            return s;
    }

    return "";
}

void
    load_asset_csv
        (std::istream& input,
         std::vector <std::pair<db_a_elmnt_t,persist::asset_operation>> &okRows,
         std::map <int, std::string> &failRows,
         touch_cb_t touch_fn,
         std::string user
         )
{
    LOG_START;

    std::vector <std::vector<cxxtools::String> > data;
    cxxtools::CsvDeserializer deserializer(input);
    if ( hasApostrof(input) ) {
        std::string msg = "CSV file contains ' (apostrof), please remove it";
        log_error("%s\n", msg.c_str());
        LOG_END;
        bios_throw("bad-request-document", msg.c_str());
    }
    char delimiter = findDelimiter(input);
    if (delimiter == '\x0') {
        std::string msg{"Cannot detect the delimiter, use comma (,) semicolon (;) or tabulator"};
        log_error("%s", msg.c_str());
        LOG_END;
        bios_throw("bad-request-document", msg.c_str());
    }
    log_debug("Using delimiter '%c'", delimiter);
    deserializer.delimiter(delimiter);
    deserializer.readTitle(false);
    deserializer.deserialize(data);
    CsvMap cm{data};
    cm.deserialize();

    cm.setCreateMode(CREATE_MODE_CSV);
    cm.setCreateUser(user);
    cm.setUpdateUser(user);
    std::time_t timestamp = std::time(NULL);
    char mbstr[100];
    if (std::strftime(mbstr, sizeof (mbstr), "%FT%T%z", std::localtime(&timestamp))) {
        cm.setUpdateTs(std::string(mbstr));
    }
    return load_asset_csv(cm, okRows, failRows, touch_fn);
}

std::pair<db_a_elmnt_t, persist::asset_operation>
    process_one_asset
        (const CsvMap& cm)
{
    LOG_START;

    auto m = mandatory_missing(cm);
    if ( m != "" )
    {
        std::string msg{"column '" + m + "' is missing, import is aborted"};
        log_error("%s", msg.c_str());
        LOG_END;
        bios_throw("request-param-required", m.c_str());
    }

    tntdb::Connection conn;
    std::string msg{"No connection to database"};
    try{
        conn = tntdb::connectCached(url);
    }
    catch(...)
    {
        log_error("%s", msg.c_str());
        LOG_END;
        bios_throw("internal-error", msg.c_str());
    }

    auto TYPES = read_element_types (conn);
    if (TYPES.empty ())
        bios_throw("internal-error", msg.c_str());

    auto SUBTYPES = read_device_types (conn);
    if (SUBTYPES.empty ())
        bios_throw("internal-error", msg.c_str());

    std::set<a_elmnt_id_t> ids{};
    size_t rc_0 = SIZE_MAX;
    auto iname = cm.get(1, "id");
    if ("rackcontroller-0" == iname) {
        rc_0 = 1;
    }
    auto ret = process_row(conn, cm, 1, TYPES, SUBTYPES, ids, true, rc_0);
    LOG_END;
    return ret;
}

void
    load_asset_csv
        (const CsvMap& cm,
         std::vector <std::pair<db_a_elmnt_t,persist::asset_operation>> &okRows,
         std::map <int, std::string> &failRows,
         touch_cb_t touch_fn
         )
{
    LOG_START;

    auto m = mandatory_missing(cm);
    if ( m != "" )
    {
        std::string msg{"column '" + m + "' is missing, import is aborted"};
        log_error("%s", msg.c_str());
        LOG_END;
        bios_throw("request-param-bad", m.c_str(), std::string("<missing column'").append(m).append("'>").c_str(),
            std::string("<column '").append(m).append("' is present in csv>").c_str());
    }

    tntdb::Connection conn;
    std::string msg{"No connection to database"};
    try{
        conn = tntdb::connectCached(url);
    }
    catch(...)
    {
        log_error("%s", msg.c_str());
        LOG_END;
        bios_throw("internal-error", msg.c_str());
    }

    auto TYPES = read_element_types (conn);

    if (TYPES.empty ())
        bios_throw("internal-error", msg.c_str());

    auto SUBTYPES = read_device_types (conn);
    if (SUBTYPES.empty ())
        bios_throw("internal-error", msg.c_str());

    // BIOS-2506
    std::set<a_elmnt_id_t> ids{};

    // if there are rackcontroller in the import, promote one of it to rackcontroller-0 if not done already
    MlmClient client;
    size_t rc0 = promote_rc0(client, cm, touch_fn);

    std::set<size_t> processedRows;
    bool somethingProcessed;
    do {
        somethingProcessed = false;
        failRows.clear ();
        for (size_t row_i = 1; row_i != cm.rows(); row_i++) {
            if (processedRows.find (row_i) != processedRows.end ()) continue;
            try{
                auto ret = process_row(conn, cm, row_i, TYPES, SUBTYPES, ids, true, rc0);
                touch_fn ();
                okRows.push_back (ret);
                log_info ("row %zu was imported successfully", row_i);
                somethingProcessed = true;
                processedRows.insert (row_i);
            }
            catch (const std::invalid_argument &e) {
                failRows.insert(std::make_pair(row_i + 1, e.what()));
                log_error ("row %zu not imported: %s", row_i, e.what());
            }
        }
    } while (somethingProcessed);
    LOG_END;
}

} // namespace persist
