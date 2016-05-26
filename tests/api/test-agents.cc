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

/*! \file   test-agents.cc
    \brief  Not yet documented file
    \author Michal Vyskocil <MichalVyskocil@Eaton.com>
    \author Alena Chernikava <AlenaChernikava@Eaton.com>
    \author Michal Hrusecky <MichalHrusecky@Eaton.com>
    \author Karol Hrdina <KarolHrdina@Eaton.com>
    \author Tomas Halman <TomasHalman@Eaton.com>
*/

#include <catch.hpp>
#include <stdio.h>

#include "log.h"
#include "agents.h"
#include "utils.h"
#include "utils_app.h"
#include "utils_ymsg.h"
#include "cleanup.h"

TEST_CASE ("bios asset extended message encode/decode", "[agents][public_api][asset_extra]")
{
    log_open ();

    const char *name = "my_test_device";
    zhash_t *ext_attributes = zhash_new();
    zhash_autofree (ext_attributes);
    zhash_insert (ext_attributes, "key1", (char*)"value1");
    zhash_insert (ext_attributes, "key2", (char*)"value2");
    zhash_insert (ext_attributes, "key3", (char*)"value3");

    uint32_t type_id = 1;
    uint32_t subtype_id = 1;
    uint32_t parent_id = 1;
    const char *status = "active";
    uint8_t priority = 2;
    int8_t operation = 1;

    _scoped_ymsg_t *encoded_message = bios_asset_extra_encode (
            name,
            &ext_attributes,
            type_id,
            subtype_id,
            parent_id,
            status,
            priority,
            operation);
    REQUIRE (ext_attributes == NULL);


    _scoped_zhash_t *ext_attributes_new = NULL;
    char *name_new = NULL;
    uint32_t type_id_new = 0;
    uint32_t subtype_id_new = 0;
    uint32_t parent_id_new = 0;
    char *status_new = NULL;
    uint8_t priority_new = 0;
    int8_t operation_new = 0;
 
    int rv = bios_asset_extra_extract (encoded_message, &name_new, 
        &ext_attributes_new, &type_id_new, &subtype_id_new, &parent_id_new, &status_new,
        &priority_new, &operation_new);
    REQUIRE ( rv == 0 );
    REQUIRE ( encoded_message != NULL );
    REQUIRE ( streq (name, name_new) == true );
    REQUIRE ( type_id == type_id_new );
    REQUIRE ( subtype_id == subtype_id_new );
    REQUIRE ( parent_id == parent_id_new );
    REQUIRE ( priority == priority_new );
    REQUIRE ( operation == operation_new );
    REQUIRE ( streq (status, status_new) == true );
    REQUIRE ( zhash_size (ext_attributes_new) == 3 );
    
    const char *value1 = (char *) zhash_lookup (ext_attributes_new, "key1");
    REQUIRE ( streq (value1, "value1") == true );

    const char *value2 = (char*) zhash_lookup (ext_attributes_new, "key2");
    REQUIRE ( streq (value2, "value2") == true );

    const char *value3 = (char *) zhash_lookup (ext_attributes_new, "key3");
    REQUIRE ( streq (value3, "value3") == true );

    FREE0 (status_new)
    FREE0 (name_new)
    ymsg_destroy (&encoded_message);
    zhash_destroy(&ext_attributes_new);
    zhash_destroy(&ext_attributes);
}
