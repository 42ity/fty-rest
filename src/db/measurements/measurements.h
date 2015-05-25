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

/*!
 \file measurements.h
 \brief high level db api for measurements
 \author Karol Hrdina <KarolHrdina@eaton.com>
*/

#ifndef SRC_DB_MEASUREMENTS_MEASUREMENTS_H__
#define SRC_DB_MEASUREMENTS_MEASUREMENTS_H__

#include <tntdb/connect.h>
#include <map>
#include <string>

namespace persist
{

// -1 err/failure , 0 ok, 1 element_id not found, 2 topic not found    
int
get_measurements
(uint64_t element_id, const char *topic, int64_t start_timestamp, int64_t end_timestamp, bool left_interval_closed, bool right_interval_closed, std::map <int64_t, double>& measurements, std::string& unit);


// TODO: change to step
int
get_measurements_averages
(uint64_t element_id, const char *topic, const char *type, int64_t start_timestamp, int64_t end_timestamp, std::map <int64_t, double>& measurements, std::string& unit);

int
get_measurements_sampled
(uint64_t element_id, const char *topic, int64_t start_timestamp, int64_t end_timestamp, std::map <int64_t, double>& measurements, std::string& unit);
    

} // namespace persist


#endif // SRC_DB_MEASUREMENTS_MEASUREMENTS_H__

