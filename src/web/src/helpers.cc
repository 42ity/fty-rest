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

#include <cassert>

#include "helpers.h"

#include "log.h"

bool
check_element_identifier (const char *param_name, const std::string& param_value, uint32_t& element_id, http_errors_t& errors) {
    assert (param_name);
    if (param_value.empty ()) {
        http_add_error (errors,"request-param-required", param_name);
        return false;
    }

    uint32_t eid = 0;
    try {
        eid = utils::string_to_element_id (param_value);
    }
    catch (const std::invalid_argument& e) {
        http_add_error (errors, "request-param-bad", param_name,
            std::string ("value '").append (param_value).append ("'").append (" is not an element identifier").c_str (),
            std::string ("an unsigned integer in range 1 to ").append (std::to_string (UINT_MAX)).append (".").c_str ());
        return false;
    }
    catch (const std::out_of_range& e) {
        http_add_error (errors, "request-param-bad", param_name,
            std::string ("value '").append (param_value).append ("'").append (" is out of range").c_str (),
            std::string ("value in range 1 to ").append (std::to_string (UINT_MAX)).append (".").c_str ());
        return false;
    }
    catch (const std::exception& e) {
        log_error ("std::exception caught: %s", e.what ());
        http_add_error (errors, "internal-error");
        return false;
    }
    element_id = eid;
    return true;
}

typedef int (t_check_func)(int letter);

bool
check_func_text (const char *param_name, const std::string& param_value, http_errors_t& errors,  size_t minlen, size_t maxlen, t_check_func func) {
    if (param_value.size () < minlen) {
        http_add_error (errors, "request-param-bad", param_name,
                        std::string ("value '").append (param_value).append ("'").append (" is too short").c_str (),
                        std::string ("string from ").append (std::to_string (minlen)).append (" to ").append (std::to_string(maxlen)).append (" characters.").c_str ());
        return false;
    }
    if (param_value.size () > maxlen) {
        http_add_error (errors, "request-param-bad", param_name,
                        std::string ("value '").append (param_value).append ("'").append (" is too long").c_str (),
                        std::string ("string from ").append (std::to_string (minlen)).append (" to ").append (std::to_string(maxlen)).append (" characters.").c_str ());
        return false;
    }
    for (const auto letter : param_value) {
        if (!func (letter)) {
        http_add_error (errors, "request-param-bad", param_name,
                        std::string ("value '").append (param_value).append ("'").append (" contains invalid characters").c_str (),
                        "valid string");
        return false;

        }

    }
    return true;
}

int isalnumdash (int c) {
    return isalnum (c) || c == '-' || c == '_'; 
}

bool
check_alnumplus_text (const char *param_name, const std::string& param_value, http_errors_t& errors, size_t minlen, size_t maxlen) {
    return check_func_text (param_name, param_value, errors, minlen, maxlen, isalnumdash);
}

bool
check_printable_text (const char *param_name, const std::string& param_value, http_errors_t& errors, size_t minlen, size_t maxlen) {
    return check_func_text (param_name, param_value, errors, minlen, maxlen, isprint);
}
