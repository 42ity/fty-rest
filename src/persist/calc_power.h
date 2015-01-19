/*
Copyright (C) 2015 Eaton
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
 
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*! \file calc_power.h
    \brief Functions for calculating a total rack and DC power from 
     database values.
    \author Alena Chernikava <alenachernikava@eaton.com>
*/

#ifndef SRC_PERSIST_CALC_POWER_H_
#define SRC_PERSIST_CALC_POWER_H_

#include <set>
#include <tuple>

#include "common_msg.h"
#include "dbtypes.h"

#include "assettopology.h"

#define DEVICE_TYPE_EPDU 3
#define DEVICE_TYPE_PDU 4
#define DEVICE_TYPE_UPS 1
#define DEVICE_TYPE_SERVER 5

// ===========================================================================
// Helper types and functions
// ===========================================================================

/**
 * \brief Type represents a structure of unique power sources for IT devices
 * that are in some rack.
 * 
 * First  -- a set of all ePDU power sources.
 * Second -- a set of all UPS power sources.
 * Third  -- a set of all IT devices (to get info directly).
 */
typedef 
        std::tuple < std::set < device_info_t >, 
                     std::set < device_info_t >, 
                     std::set < device_info_t>  
                    >
        power_sources_t;


/**
 * \brief Checks the type of device.
 *
 * \param device - a device to be checked.
 *
 * \return true  if it is an epdu.
 *         false if it is not an epdu.
 */
bool is_epdu (const device_info_t &device);


/**
 * \brief Checks the type of device.
 *
 * \param device - a device to be checked.
 *
 * \return true  if it is a pdu.
 *         false if it is not a pdu.
 */
bool is_pdu (const device_info_t &device);


/**
 * \brief Checks the type of device.
 *
 * \param device - a device to be checked.
 *
 * \return true  if it is a ups.
 *         false if it is not a ups.
 */
bool is_ups (const device_info_t &device);


/**
 * \brief Checks the type of device.
 *
 * \param device - a device to be checked.
 *
 * \return true  if it is an IT device.
 *         false if it is not an IT device.
 */
bool is_it_device (const device_info_t &device);


//TODO move device_info_t to map
/**
 * \brief Extracts power sources from power topology and sort them by device type into three cathegories.
 *
 * \param url - connection to a database
 * \param power_topology - a power topology represented as a pair (set of devices + set of powelinks)
 * \param start_device - a device we look sources for
 *
 * \result a tuple:
 *              first  - set of epdu
 *              second - set of ups
 *              third  - set of devices
 */
power_sources_t
    extract_power_sources ( const char* url,
          std::pair < std::set < device_info_t >, 
                      std::set < powerlink_info_t > > power_topology, 
          device_info_t start_device );









zmsg_t* calc_total_rack_power (const char *url, a_elmnt_id_t rack_element_id);
#endif //SRC_PERSIST_CALC_POWER_H_
