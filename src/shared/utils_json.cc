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

#include "shared/utils_json.h"
#include "shared/data.h"
#include "shared/utils.h"
#include "shared/utilspp.h"
#include "web/src/asset_computed_impl.h"
#include <cmath>
#include <fty_common.h>
#include <fty_common_db_asset.h>
#include <fty_common_rest.h>
#include <fty_proto.h>
#include <fty_shm.h>
#include <regex>

struct Outlet
{
    std::string label;
    bool        label_r;
    std::string type;
    bool        type_r;
    std::string group;
    bool        group_r;
};

std::string getOutletNumber(const std::string& extAttributeName)
{
    auto        dot1    = extAttributeName.find_first_of(".");
    std::string oNumber = extAttributeName.substr(dot1 + 1);
    auto        dot2    = oNumber.find_first_of(".");
    oNumber             = oNumber.substr(0, dot2);
    return oNumber;
}

// encode metric GET request

zmsg_t* s_rt_encode_GET(const char* name)
{
    static const char* method = "GET";

    zuuid_t* uuid = zuuid_new();
    zmsg_t*  msg  = zmsg_new();

    zmsg_pushmem(msg, zuuid_data(uuid), zuuid_size(uuid));
    zuuid_destroy(&uuid);
    zmsg_addstr(msg, method);
    zmsg_addstr(msg, name);
    return msg;
}

double s_rack_realpower_nominal(mlm_client_t* /*client*/, const std::string& name)
{
    double      ret = 0.0;
    std::string value;
    if (fty::shm::read_metric_value(name, "realpower.nominal", value) != 0) {
        log_warning("No realpower.nominal for '%s'", name.c_str());
    } else {
        try {
            ret = std::stod(value);
        } catch (const std::exception& e) {
            log_error(
                "the metric returned a string that does not encode a double value: '%s'. Defaulting to 0.0 value.",
                value.c_str());
            ret = std::nan("");
        }
    }

    return ret;
    //
    //  zmsg_t *request = s_rt_encode_GET(name.c_str());
    //  mlm_client_sendto(client, "fty-metric-cache", "latest-rt-data", NULL, 1000, &request);
    //
    //  //TODO: this intentionally wait forewer, to be fixed by proper client pool
    //  zmsg_t *msg = mlm_client_recv(client);
    //  if (!msg)
    //    throw std::runtime_error("no reply from broker!");
    //
    //  //TODO: check if we have right uuid, to be fixed by proper client pool
    //  char *uuid = zmsg_popstr(msg);
    //  zstr_free(&uuid);
    //
    //  char *result = zmsg_popstr(msg);
    //  if (NULL == result || !streq(result, "OK"))
    //  {
    //    log_warning("Error reply for device '%s', result=%s", name.c_str(), result);
    //    if (NULL != result)
    //    {
    //      zstr_free(&result);
    //    }
    //    if (NULL != msg)
    //    {
    //      zmsg_destroy(&msg);
    //    }
    //    return ret;
    //  }
    //
    //  char *element = zmsg_popstr(msg);
    //  if (!streq(element, name.c_str()))
    //  {
    //    log_warning("element name (%s) from message differs from requested one (%s), ignoring", element,
    //    name.c_str()); zstr_free(&element); zmsg_destroy(&msg); return ret;
    //  }
    //  zstr_free(&element);
    //
    //  zmsg_t *data = zmsg_popmsg(msg);
    //  while (data)
    //  {
    //    fty_proto_t *bmsg = fty_proto_decode(&data);
    //    if (!bmsg)
    //    {
    //      log_warning("decoding fty_proto_t failed");
    //      continue;
    //    }
    //
    //    if (!streq(fty_proto_type(bmsg), "realpower.nominal"))
    //    {
    //      fty_proto_destroy(&bmsg);
    //      data = zmsg_popmsg(msg);
    //      continue;
    //    }
    //    else
    //    {
    //      ret = std::stod(fty_proto_value(bmsg));
    //      fty_proto_destroy(&bmsg);
    //      break;
    //    }
    //  }
    //  zmsg_destroy(&msg);
    //  return ret;
}

std::string getJsonAlert(tntdb::Connection connection, fty_proto_t* alert)
{
    std::string json = "";

    char     buff[64];
    uint64_t timestamp = fty_proto_aux_number(alert, "ctime", fty_proto_time(alert));
    int      rv        = calendar_to_datetime(time_t(timestamp), buff, 64);
    if (rv == -1) {
        log_error("can't convert %" PRIu64 "to calendar time, skipping element '%s'", timestamp, fty_proto_rule(alert));
        return json;
    }

    auto asset_element = DBAssets::select_asset_element_web_byName(connection, fty_proto_name(alert));
    if (asset_element.status != 1) {
        log_error("%s", asset_element.msg.c_str());
        return json;
    }

    log_debug("DBAssets::id_to_name_ext_name ('%d')", asset_element.item.id);
    std::pair<std::string, std::string> asset_element_names = DBAssets::id_to_name_ext_name(asset_element.item.id);
    if (asset_element_names.first.empty() && asset_element_names.second.empty()) {
        log_error("internal-error : Database error");
        return json;
    }

    // TBD Workaround for IPMPROG-1729: Replace all occurrences of ename value with correct friendly name
    // ... "ename": { "value":"<ename_to_replace>", "assetLink":"ups-xxxxxx" } ...
    auto updateDescription = [asset_element_names](std::string description) -> std::string {
        // caution: inverse search for regex_match (last found in first)
        const static std::regex reg(R"xxx((.*\ename\")([:{ ]*\"value\"[:\" ]*)([^\"]*)(\".*))xxx");
        std::string res;
        std::smatch matches;
        std::string search = description;
        while (std::regex_match(search, matches, reg) && matches.size() == 5) {
            //logDebug("match1={} match2={} match3={} match4={}", matches.str(1), matches.str(2), matches.str(3), matches.str(4));
            res = matches.str(2) + asset_element_names.second + matches.str(4) + res;
            search = matches.str(1);
        }
        res = search + res;
        return res;
    };

    json += "{";

    json += utils::json::jsonify("timestamp", buff) + ",";
    json += utils::json::jsonify("rule_name", fty_proto_rule(alert)) + ",";
    json += utils::json::jsonify("element_id", fty_proto_name(alert)) + ",";
    json += utils::json::jsonify("element_name", asset_element_names.second) + ",";
    json += utils::json::jsonify("element_type", asset_element.item.type_name) + ",";
    json += utils::json::jsonify("element_sub_type", utils::strip(asset_element.item.subtype_name)) + ",";
    json += utils::json::jsonify("state", fty_proto_state(alert)) + ",";
    json += utils::json::jsonify("severity", fty_proto_severity(alert)) + ",";
    json += utils::json::jsonify("description", updateDescription(fty_proto_description(alert)));
    const char* md = fty_proto_metadata(alert); // assume json object payload if !empty
    json += "," + utils::json::jsonify("metadata", ((md && (*md)) ? md : "{}"));
    json += "}";

    return json;
}

std::string getJsonAsset(mlm_client_t* clientMlm, int64_t elemId)
{
    std::string json;
    // Get informations from database
    asset_manager asset_mgr;
    auto          tmp = asset_mgr.get_item1(uint32_t(elemId));
    if (tmp.status == 0) {
        switch (tmp.errsubtype) {
            case DB_ERROR_NOTFOUND:
                log_error("element-not-found : %" PRId64 "", elemId);
            case DB_ERROR_BADINPUT:
            case DB_ERROR_INTERNAL:
            default:
                log_error("get_item1 Internal database error");
        }
        return json;
    }

    std::pair<std::string, std::string> parent_names    = DBAssets::id_to_name_ext_name(tmp.item.basic.parent_id);
    std::string                         parent_name     = parent_names.first;
    std::string                         ext_parent_name = parent_names.second;

    std::pair<std::string, std::string> asset_names = DBAssets::id_to_name_ext_name(tmp.item.basic.id);
    if (asset_names.first.empty() && asset_names.second.empty()) {
        log_error("Database failure");
        return json;
    }
    std::string asset_ext_name = asset_names.second;

    json += "{";

    json += utils::json::jsonify("id", tmp.item.basic.name) + ",";
    json += "\"power_devices_in_uri\": \"/api/v1/assets\?in=";
    json += (tmp.item.basic.name);
    json += "&sub_type=epdu,pdu,feed,genset,ups,sts,rackcontroller\",";
    json += utils::json::jsonify("name", asset_ext_name) + ",";
    json += utils::json::jsonify("status", tmp.item.basic.status) + ",";
    json += utils::json::jsonify("priority", "P" + std::to_string(tmp.item.basic.priority)) + ",";
    json += utils::json::jsonify("type", tmp.item.basic.type_name) + ",";

    // if element is located, then show the location
    if (tmp.item.basic.parent_id != 0) {
        json += utils::json::jsonify("location_uri", "/api/v1/asset/" + parent_name) + ",";
        json += utils::json::jsonify("location_id", parent_name) + ",";
        json += utils::json::jsonify("location", ext_parent_name) + ",";

        // Get informations from database
        auto parentAsset = asset_mgr.get_item1(tmp.item.basic.parent_id);
        json += utils::json::jsonify("location_type", parentAsset.item.basic.type_name) + ",";
    } else {
        json += "\"location\":\"\",";
        json += "\"location_type\":\"\",";
    }

    json += "\"groups\": [";
    // every element (except groups) can be placed in some group
    if (!tmp.item.groups.empty()) {
        uint32_t    group_count = uint32_t(tmp.item.groups.size());
        uint32_t    i           = 1;
        std::string ext_name    = "";
        for (auto& oneGroup : tmp.item.groups) {
            std::pair<std::string, std::string> group_names = DBAssets::id_to_name_ext_name(oneGroup.first);
            if (group_names.first.empty() && group_names.second.empty()) {
                log_error("Database failure");
                json = "";
                return json;
            }
            ext_name = group_names.second;
            json += "{";
            json += utils::json::jsonify("id", oneGroup.second) + ",";
            json += utils::json::jsonify("name", ext_name);
            json += "}";

            if (i != group_count) {
                json += ",";
            }
            i++;
        }
    }
    json += "]";

    // Device is special element with more attributes
    if (tmp.item.basic.type_id == persist::asset_type::DEVICE) {
        json += ", \"powers\": [";

        if (!tmp.item.powers.empty()) {
            uint32_t power_count = uint32_t(tmp.item.powers.size());
            uint32_t i           = 1;
            for (auto& oneLink : tmp.item.powers) {
                std::pair<std::string, std::string> link_names = DBAssets::id_to_name_ext_name(oneLink.src_id);
                if (link_names.first.empty() && link_names.second.empty()) {
                    log_error("Database failure");
                    json = "";
                    return json;
                }
                json += "{";
                json += utils::json::jsonify("src_name", link_names.second) + ",";
                json += utils::json::jsonify("src_id", oneLink.src_name);

                if (!oneLink.src_socket.empty()) {
                    json += "," + utils::json::jsonify("src_socket", oneLink.src_socket);
                }
                if (!oneLink.dest_socket.empty()) {
                    json += "," + utils::json::jsonify("dest_socket", oneLink.dest_socket);
                }

                json += "}";
                if (i != power_count) {
                    json += ",";
                }
                i++;
            }
        }

        json += "]";
    }
    // ACE: to be consistent with RFC-11 this was put here
    if (tmp.item.basic.type_id == persist::asset_type::GROUP) {
        auto it = tmp.item.ext.find("type");
        if (it != tmp.item.ext.end()) {
            json += "," + utils::json::jsonify("sub_type", utils::strip(it->second.first));
            tmp.item.ext.erase(it);
        }
    } else {
        json += "," + utils::json::jsonify("sub_type", utils::strip(tmp.item.basic.subtype_name));

        json += ", \"parents\" : [";
        size_t i = 1;

        std::string ext_name = "";

        for (const auto& it : tmp.item.parents) {
            char                                comma    = i != tmp.item.parents.size() ? ',' : ' ';
            std::pair<std::string, std::string> it_names = DBAssets::id_to_name_ext_name(std::get<0>(it));
            if (it_names.first.empty() && it_names.second.empty()) {
                log_error("Database failure");
                json = "";
                return json;
            }
            ext_name = it_names.second;
            json += "{";
            json += utils::json::jsonify("id", std::get<1>(it));
            json += "," + utils::json::jsonify("name", ext_name);
            json += "," + utils::json::jsonify("type", std::get<2>(it));
            json += "," + utils::json::jsonify("sub_type", std::get<3>(it));
            json += "}";
            json += (comma);
            i++;
        }
        json += "]";
    }

    json += ", \"ext\" : [";
    bool isExtCommaNeeded = false;

    if (!tmp.item.basic.asset_tag.empty()) {
        json += "{";
        json += utils::json::jsonify("asset_tag", tmp.item.basic.asset_tag);
        json += ", \"read_only\" : false}";
        isExtCommaNeeded = true;
    }

    std::map<std::string, Outlet> outlets;
    std::vector<std::string>      ips;
    std::vector<std::string>      macs;
    std::vector<std::string>      fqdns;
    std::vector<std::string>      hostnames;
    if (!tmp.item.ext.empty()) {
        std::regex r_outlet_label("^outlet\\.[0-9][0-9]*\\.label$");
        std::regex r_outlet_group("^outlet\\.[0-9][0-9]*\\.group$");
        std::regex r_outlet_type("^outlet\\.[0-9][0-9]*\\.type$");
        std::regex r_ip("^ip\\.[0-9][0-9]*$");
        std::regex r_mac("^mac\\.[0-9][0-9]*$");
        std::regex r_hostname("^hostname\\.[0-9][0-9]*$");
        std::regex r_fqdn("^fqdn\\.[0-9][0-9]*$");
        for (auto& oneExt : tmp.item.ext) {
            auto& attrName = oneExt.first;

            if (attrName == "name")
                continue;

            // filter location_type (already present)
            if (attrName == "location_type")
                continue;

            auto& attrValue  = oneExt.second.first;
            auto  isReadOnly = oneExt.second.second;
            if (std::regex_match(attrName, r_outlet_label)) {
                auto oNumber = getOutletNumber(attrName);
                auto it      = outlets.find(oNumber);
                if (it == outlets.cend()) {
                    auto r = outlets.emplace(oNumber, Outlet());
                    it     = r.first;
                }
                it->second.label   = attrValue;
                it->second.label_r = isReadOnly;
                continue;
            } else if (std::regex_match(attrName, r_outlet_group)) {
                auto oNumber = getOutletNumber(attrName);
                auto it      = outlets.find(oNumber);
                if (it == outlets.cend()) {
                    auto r = outlets.emplace(oNumber, Outlet());
                    it     = r.first;
                }
                it->second.group   = attrValue;
                it->second.group_r = isReadOnly;
                continue;
            } else if (std::regex_match(attrName, r_outlet_type)) {
                auto oNumber = getOutletNumber(attrName);
                auto it      = outlets.find(oNumber);
                if (it == outlets.cend()) {
                    auto r = outlets.emplace(oNumber, Outlet());
                    it     = r.first;
                }
                it->second.type   = attrValue;
                it->second.type_r = isReadOnly;
                continue;
            } else if (std::regex_match(attrName, r_ip)) {
                ips.push_back(attrValue);
                continue;
            } else if (std::regex_match(attrName, r_mac)) {
                macs.push_back(attrValue);
                continue;
            } else if (std::regex_match(attrName, r_fqdn)) {
                fqdns.push_back(attrValue);
                continue;
            } else if (std::regex_match(attrName, r_hostname)) {
                hostnames.push_back(attrValue);
                continue;
            }
            // If we are here -> then this attribute is not special and should be returned as "ext"
            std::string extKey = oneExt.first;
            std::string extVal = oneExt.second.first;

            json += isExtCommaNeeded ? "," : "";
            json += "{";
            json += utils::json::jsonify(extKey, extVal) + ",\"read_only\" :";
            json += oneExt.second.second ? "true" : "false";
            json += "}";

            isExtCommaNeeded = true;
        } // end of for each loop for ext attribute
    }

    json += "]";
    // Print "ips"

    if (!ips.empty()) {
        json += ", \"ips\" : [";
        int i = 1;

        int max_size = int(ips.size());

        for (const auto& oneIp : ips) {
            json += "\"" + (oneIp) + "\"";
            json += (max_size == i ? "" : ",");
            i++;
        }
        json += "]";
    }

    // Print "macs"
    if (!macs.empty()) {

        json += ", \"macs\" : [";
        int i = 1;

        int max_size = int(macs.size());

        for (auto& oneMac : macs) {
            json += "\"" + (oneMac) + "\"";
            json += (max_size == i ? "" : ",");
            i++;
        }
        json += "]";
    }

    // Print "fqdns"
    if (!fqdns.empty()) {
        json += ", \"fqdns\" : [";
        int i        = 1;
        int max_size = int(fqdns.size());
        for (auto& oneFqdn : fqdns) {
            json += "\"" + (oneFqdn) + "\"";
            json += (max_size == i ? "" : ",");
            i++;
        }
        json += "]";
    }

    // Print "fqdns"
    if (!hostnames.empty()) {

        json += " , \"hostnames\" : [";
        int i = 1;

        int max_size = int(hostnames.size());
        for (auto& oneHostname : hostnames) {
            json += "\"" + (oneHostname) + "\"";
            json += (max_size == i ? "" : ",");
            i++;
        }
        json += "]";
    }

    // Print "outlets"
    if (!outlets.empty()) {
        json += ", \"outlets\": {";
        int i = 1;

        int max_size = int(outlets.size());

        for (auto& oneOutlet : outlets) {

            bool isCommaNeeded = false;

            json += "\"" + (oneOutlet.first) + "\" : [";
            if (!oneOutlet.second.label.empty()) {

                std::string outletLabel = oneOutlet.second.label;

                json += "{\"name\":\"label\",";
                json += utils::json::jsonify("value", outletLabel) + ",\"read_only\" : ";
                json += oneOutlet.second.label_r ? "true" : "false";
                json += "}";
                isCommaNeeded = true;
            }

            if (!oneOutlet.second.group.empty()) {
                if (isCommaNeeded) {
                    json += ",";
                }

                json += "{\"name\":\"group\",";
                json += utils::json::jsonify("value", oneOutlet.second.group) + ",\"read_only\" : ";
                json += oneOutlet.second.group_r ? "true" : "false";
                json += "}";
                isCommaNeeded = true;
            }

            if (!oneOutlet.second.type.empty()) {
                if (isCommaNeeded) {
                    json += ",";
                }

                json += "{\"name\":\"type\",";
                json += utils::json::jsonify("value", oneOutlet.second.type) + ",\"read_only\" : ";
                json += oneOutlet.second.type_r ? "true" : "false";
                json += "}";
            }

            json += "]";
            json += (max_size == i ? "" : ",");
            i++;
        }
        json += "}";
    }

    json += ", \"computed\" : {";
    if (persist::is_rack(tmp.item.basic.type_id)) {
        int    freeusize         = free_u_size(tmp.item.basic.id);
        double realpower_nominal = s_rack_realpower_nominal(clientMlm, tmp.item.basic.name.c_str());

        json += "\"freeusize\":" + (freeusize >= 0 ? std::to_string(freeusize) : "null");
        json +=
            ",\"realpower.nominal\":" + (!std::isnan(realpower_nominal) ? std::to_string(realpower_nominal) : "null");
        json += ", \"outlet.available\" : {";
        std::map<std::string, int> res;
        int                        rv = rack_outlets_available(tmp.item.basic.id, res);
        if (rv != 0) {
            log_error("Database failure");
            json = "";
            return json;
        }
        size_t i = 1;
        for (const auto& it : res) {

            std::string val   = it.second >= 0 ? std::to_string(it.second) : "null";
            std::string comma = i == res.size() ? "" : ",";
            i++;
            json += "\"" + it.first + "\":" + val;
            json += (comma);
        } // for it : res

        json += "}";
    } // rack

    json += "}}";
    return json;
}
