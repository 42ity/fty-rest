/*
 *
 * Copyright (C) 2015 Eaton
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

/*!
 * \file dci_impl.h
 * \author Michal Vyskocil <MichalVyskocil@Eaton.com>
 * \author Alena Chernikava <AlenaChernikava@Eaton.com>
 * \brief  Helper functions for datacenter metrics
 *
 * This file exists only to have syntax highlighting correct.
 * To be included in datacenter_indicators.ecpp
 */
#ifndef SRC_WEB_SRC_DCI_IMPL_H
#define SRC_WEB_SRC_DCI_IMPL_H

#include <map>
#include <string>

#include "db/measurements.h"
#include "utils++.h"


// ATTENTION!!! for trends:
//  first one should be average, second one is raw measurement
static const std::map<std::string, const std::string> PARAM_TO_SRC = {
    {"power", "realpower.default"},
    {"avg_power_last_day", "realpower.default_arithmetic_mean_24h"},
    {"avg_power_last_week", "realpower.default_arithmetic_mean_7d"},
    {"avg_power_last_month", "realpower.default_arithmetic_mean_30d"},

    {"min_power_last_day", "realpower.default_min_24h"},
    {"min_power_last_week", "realpower.default_min_7d"},
    {"min_power_last_month", "realpower.default_min_30d"},

    {"max_power_last_day", "realpower.default_max_24h"},
    {"max_power_last_week", "realpower.default_max_7d"},
    {"max_power_last_month", "realpower.default_max_30d"},

    {"trend_power_last_day", "realpower.default_arithmetic_mean_24h/realpower.default"},
    {"trend_power_last_week", "realpower.default_arithmetic_mean_7d/realpower.default"},
    {"trend_power_last_month", "realpower.default_arithmetic_mean_30d/realpower.default"},

    {"temperature", "average.temperature"},
    {"avg_temperature_last_day",  "average.temperature_arithmetic_mean_24h"},
    {"avg_temperature_last_week", "average.temperature_arithmetic_mean_7d"},
    {"avg_temperature_last_month", "average.temperature_arithmetic_mean_30d"},

    {"min_temperature_last_day",  "average.temperature_min_24h"},
    {"min_temperature_last_week", "average.temperature_min_7d"},
    {"min_temperature_last_month", "average.temperature_min_30d"},

    {"max_temperature_last_day",  "average.temperature_max_24h"},
    {"max_temperature_last_week", "average.temperature_max_7d"},
    {"max_temperature_last_month", "average.temperature_max_30d"},

    {"trend_temperature_last_day",  "average.temperature_arithmetic_mean_24h/average.temperature"},
    {"trend_temperature_last_week", "average.temperature_arithmetic_mean_7d/average.temperature"},
    {"trend_temperature_last_month", "average.temperature_arithmetic_mean_30d/average.temperature"},

    {"humidity", "average.humidity"},
    {"avg_humidity_last_day", "average.humidity_arithmetic_mean_24h"},
    {"avg_humidity_last_week", "average.humidity_arithmetic_mean_7d"},
    {"avg_humidity_last_month", "average.humidity_arithmetic_mean_30d"},

    {"min_humidity_last_day", "average.humidity_min_24h"},
    {"min_humidity_last_week", "average.humidity_min_7d"},
    {"min_humidity_last_month", "average.humidity_min_30d"},

    {"max_humidity_last_day", "average.humidity_max_24h"},
    {"max_humidity_last_week", "average.humidity_max_7d"},
    {"max_humidity_last_month", "average.humidity_max_30d"},

    {"trend_humidity_last_day", "average.humidity_arithmetic_mean_24h/average.humidity"},
    {"trend_humidity_last_week", "average.humidity_arithmetic_mean_7d/average.humidity"},
    {"trend_humidity_last_month", "average.humidity_arithmetic_mean_30d/average.humidity"},
};


static double
    get_dc_raw(
        tntdb::Connection& conn,
        const std::string& src,
        a_elmnt_id_t id,
        std::map<const std::string, double> &cache)
{
    // step in seconds
    int step = 0;
    if ( src.find("24h") != std::string::npos ) {
        step = 24*60*60;
    }
    else if ( src.find("7d") != std::string::npos ) {
        step = 7*24*60*60 + 10*60; // ASSUMPTION 1 measurement in 7d; if one measurement each day -> remove 7!
    }
    else if ( src.find("30d") != std::string::npos ) {
        step = 30*24*60*60 + 10*60; // ASSUMPTION 1 measurement in 30; if one measurement each day -> remove 30!
    }
    else if ( src.find("15m") != std::string::npos ) {
        step = 60*15;
    }
    log_debug ("step = %d", step);
    double value = 0;
    if ( step != 0 ) {
        // here we are, if we are looking for some aggregated data
        // BIOS-1553 BUG:
        // for 24h average the following is true: we know the exact time, when the average SHOULD be
        // for 7d and for 30d average: we DON'T KNOW IT!!!! it can be any midnight in the 
        // interval (NOW -7day, NOW) / (NOW-30Day, NOW)
        // fix 7d and 30d computation with this HACKY SOLUTION
        // TODO: remove the hack, when proper way of 7d and 30d computation would work
        if ( src.find("24h") != std::string::npos ) {
            int rv = persist::select_last_aggregated_by_element_by_src_by_step(conn, id, src, step, value, false);
            if ( rv != 0 ) {
                log_debug ("not computed, take 0");
            }
        }
        else {
            //  minutes back
            m_msrmnt_value_t val = 0;
            m_msrmnt_scale_t scale = 0;
            // step in seconds, need in minutes -> step * 60
            reply_t reply = persist::select_measurement_last_web_byElementId (conn, src, id, val, scale, step * 60);
            if ( reply.rv != 0 ) {
                log_debug ("not computed, take 0");
            }
            else {
                value = val * pow(10, scale);
            }
        }
     }
    else {
        // not an aggregate -> current -> 10 minutes back
        m_msrmnt_value_t val = 0;
        m_msrmnt_scale_t scale = 0;
        reply_t reply = persist::select_measurement_last_web_byElementId (conn, src, id, val, scale, 10);
        if ( reply.rv != 0 ) {
            log_debug ("not computed, take 0");
        }
        else {
            value = val * pow(10, scale);
        }
    }
    cache.insert(std::make_pair(src, value));
    return value;
}


static double
get_dc_trend(
    tntdb::Connection& conn,
    const std::string& src,
    a_elmnt_id_t id,
    std::map<const std::string, double> &cache)
{
    if (src == "<zero>") {
        return 0.0f;
    }

    std::vector<std::string> items;
    cxxtools::split('/', src, std::back_inserter(items));
    if( items.size() != 2 ) {
        return 0.0f;
    }

    double value_actual = 0.0f;
    auto it = cache.find(items.at(1));
    if ( it != cache.cend() ) {
        value_actual = it->second;
    }
    else {
        value_actual = get_dc_raw (conn, items.at(1), id, cache);
        cache.insert(std::make_pair(src, value_actual));
    }

    double value_average = 0.0f;
    it = cache.find(items.at(0));
    if ( it != cache.cend() ) {
        value_average = it->second;
    }
    else {
        value_average = get_dc_raw(conn, items.at(0), id, cache);
        cache.insert(std::make_pair(src, value_average));
    }

    double val = 0.0f;
    if ( value_average != 0 ) {
        val = round( (value_actual - value_average ) / ( value_average ) * 1000.0f ) / 10.0f ;
    }
    cache.insert(std::make_pair(src, val));
    return val;
}


double
get_dc_indicator(
    tntdb::Connection& conn,
    const std::string& key,
    a_elmnt_id_t id,
    std::map<const std::string, double> &cache)
{
    // Assumption: key is ok.
    const std::string src = PARAM_TO_SRC.at(key); //XXX: operator[] does not work here!

    auto it = cache.find(src);
    if ( it != cache.cend() ) {
        return it->second;
    }

    if (src == "<zero>") {
        cache.insert(std::make_pair(key, 0.0f));
        return 0.0f;
    }

    if (key.substr(0,5) == "trend") {
        return get_dc_trend(conn, src, id, cache);
    }
    return get_dc_raw(conn, src, id, cache);
}

bool
s_is_valid_param(const std::string& p)
{
    return PARAM_TO_SRC.count(p) != 0;
}

std::string
s_get_valid_param (void)
{
    return utils::join_keys_map (PARAM_TO_SRC, ", ");
}


#endif // SRC_WEB_SRC_DCI_IMPL_H
