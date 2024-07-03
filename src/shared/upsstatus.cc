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

/*!
 * \file upsstatus.cc
 * \author Tomas Halman <TomasHalman@Eaton.com>
 * \author Karol Hrdina <KarolHrdina@Eaton.com>
 * \brief Not yet documented file
 */
#include "shared/upsstatus.h"
#include "shared/utils.h"
#include <iostream>
#include <string.h>

/// following definition is taken as fty-nut/lib/src/ups_status.cc

#define STATUS_CAL     (1 << 0)  //!< calibration
#define STATUS_TRIM    (1 << 1)  //!< SmartTrim
#define STATUS_BOOST   (1 << 2)  //!< SmartBoost
#define STATUS_OL      (1 << 3)  //!< on line
#define STATUS_OB      (1 << 4)  //!< on battery
#define STATUS_OVER    (1 << 5)  //!< overload
#define STATUS_LB      (1 << 6)  //!< low battery
#define STATUS_RB      (1 << 7)  //!< replace battery
#define STATUS_BYPASS  (1 << 8)  //!< on bypass
#define STATUS_OFF     (1 << 9)  //!< ups is off
#define STATUS_CHRG    (1 << 10) //!< charging
#define STATUS_DISCHRG (1 << 11) //!< discharging
#define STATUS_HB      (1 << 12) //!< High battery
#define STATUS_FSD     (1 << 13) //!< Forced Shutdown

/**
 * Status lookup table
 */

typedef struct
{
    const char* status_str{nullptr};    //!< ups.status string
    int         status_value{0};        //!< ups.status flag bit
} status_lkp_t;

status_lkp_t status_info[] = {
    {"CAL", STATUS_CAL}, {"TRIM", STATUS_TRIM}, {"BOOST", STATUS_BOOST},
    {"OL", STATUS_OL},   {"OB", STATUS_OB},     {"OVER", STATUS_OVER},
    {"LB", STATUS_LB},   {"RB", STATUS_RB},     {"BYPASS", STATUS_BYPASS},
    {"OFF", STATUS_OFF}, {"CHRG", STATUS_CHRG}, {"DISCHRG", STATUS_DISCHRG},
    {"HB", STATUS_HB},   {"FSD", STATUS_FSD},   {"NULL", 0},
};

namespace shared {

std::string upsstatus_to_string(uint16_t status)
{
    std::string result = "";
    int         bit    = 1;
    for (unsigned int i = 0; i < sizeof(status_info) / sizeof(status_lkp_t) - 1; ++i) {
        if (status & bit) {
            result += std::string(result.empty() ? "" : " ") + status_info[i].status_str;
        }
        bit <<= 1;
    }
    return result;
}

} // namespace shared
