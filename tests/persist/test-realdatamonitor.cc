#include <catch.hpp>

#include <set>
#include <iostream>
#include <czmq.h>

#include <tntdb/connect.h>
#include <tntdb/row.h>
#include <tntdb/result.h>

#include "common_msg.h"
#include "dbhelpers.h"
#include "persist_error.h"
#include "monitor.h"
#include "defs.h"
#include "dbpath.h"
/* tests for real monitoring assumed, that DB has the following state
 * In case, the initial data should be changed, the following tests should be changed too

MariaDB [box_utf8]> select * from v_bios_client_info_measurements;
+----+-----------+----------------------+---------------------+--------+-----------+-------+
| id | id_client | id_discovered_device | timestamp           | id_key | id_subkey | value |
+----+-----------+----------------------+---------------------+--------+-----------+-------+
|  1 |         4 |                    2 | 2014-11-12 09:45:59 |      1 |         1 |    32 |
|  2 |         4 |                    2 | 2014-11-12 09:46:59 |      1 |         1 |     3 |
|  3 |         4 |                    2 | 2014-11-12 09:47:59 |      2 |         1 |    31 |
|  4 |         4 |                    2 | 2014-11-12 09:48:59 |      2 |         2 |    12 |
|  5 |         4 |                    2 | 2014-11-12 09:49:59 |      1 |         2 |     1 |
|  6 |         4 |                    2 | 2014-11-12 09:59:59 |      3 |         1 |     1 |
|  7 |         4 |                    2 | 2014-11-12 09:59:59 |      7 |         1 |     2 |
|  8 |         4 |                    2 | 2014-11-12 09:59:59 |      4 |         1 |    56 |
|  9 |         4 |                    2 | 2014-11-12 09:59:59 |      5 |         1 |    17 |
| 10 |         4 |                    2 | 2014-11-12 09:59:59 |      6 |         1 |   931 |
| 11 |         4 |                    3 | 2014-11-12 09:59:59 |      3 |         5 |  2405 |
| 12 |         4 |                    3 | 2014-11-12 09:59:59 |      3 |         6 |  2405 |
| 13 |         4 |                    3 | 2014-11-12 09:59:59 |      3 |         7 |   500 |
+----+-----------+----------------------+---------------------+--------+-----------+-------+

MariaDB [box_utf8]> select * from v_bios_client_info_measurements_last;
+----+----------------------+--------+-----------+-------+---------------------+-------+-----------------------+
| id | id_discovered_device | id_key | id_subkey | value | timestamp           | scale | name                  |
+----+----------------------+--------+-----------+-------+---------------------+-------+-----------------------+
|  2 |                    2 |      1 |         1 |     3 | 2014-11-12 09:46:59 |    -1 | select_device         |
|  3 |                    2 |      2 |         1 |    31 | 2014-11-12 09:47:59 |    -1 | select_device         |
|  4 |                    2 |      2 |         2 |    12 | 2014-11-12 09:48:59 |    -1 | select_device         |
|  5 |                    2 |      1 |         2 |     1 | 2014-11-12 09:49:59 |    -1 | select_device         |
|  6 |                    2 |      3 |         1 |     1 | 2014-11-12 09:59:59 |    -1 | select_device         |
|  7 |                    2 |      7 |         1 |     2 | 2014-11-12 09:59:59 |     0 | select_device         |
|  8 |                    2 |      4 |         1 |    56 | 2014-11-12 09:59:59 |    -1 | select_device         |
|  9 |                    2 |      5 |         1 |    17 | 2014-11-12 09:59:59 |    -1 | select_device         |
| 10 |                    2 |      6 |         1 |   931 | 2014-11-12 09:59:59 |    -1 | select_device         |
| 11 |                    3 |      3 |         5 |  2405 | 2014-11-12 09:59:59 |    -1 | monitor_asset_measure |
| 12 |                    3 |      3 |         6 |  2405 | 2014-11-12 09:59:59 |    -1 | monitor_asset_measure |
| 13 |                    3 |      3 |         7 |   500 | 2014-11-12 09:59:59 |    -1 | monitor_asset_measure |
+----+----------------------+--------+-----------+-------+---------------------+-------+-----------------------+

"keytag_id:subkeytag_id:value:scale"

*/

/*
MariaDB [box_utf8]> select * from v_bios_discovered_device;
+----+-----------------------+----------------+
| id | name                  | id_device_type |
+----+-----------------------+----------------+
|  1 | DUMMY_DEVICE          |              1 |
|  2 | select_device         |              1 |
|  3 | monitor_asset_measure |              1 |
|  4 | measures              |              1 |
+----+-----------------------+----------------+
*/

//DUMMY_DEVICE_ID defined in defs.h
#define SELECT_DEVICE_ID 2
#define MONITOR_ASSET_MEASURE_ID 3
#define MEASURES_DEVICE_ID 4
#define NO_DEVICE_ID 42

static const std::set<std::string> EXP{"6:1:931:-1", "5:1:17:-1", "4:1:56:-1", "7:1:2:0", "3:1:1:-1", "1:2:1:-1", "2:2:12:-1", "2:1:31:-1", "1:1:3:-1"};

TEST_CASE("real_measurements: select_last_measurements", "[db][select][lastmeasurements]")
{
    //SUCCESS
    uint32_t id = SELECT_DEVICE_ID;
    std::string name;
    tntdb::Connection conn;
    REQUIRE_NOTHROW (conn = tntdb::connectCached(url.c_str()));
    zlist_t* measurements = select_last_measurements (conn, id);
    REQUIRE ( measurements );
    //TODO
   /* CHECK (name == "select_device");

    REQUIRE (zlist_size(measurements) == EXP.size());
    std::set<std::string> results;
    for (char* s = (char*) zlist_first(measurements); s != NULL; s = (char*) zlist_next(measurements)) {
        results.insert(s);
    }
    REQUIRE (results == EXP);
    zlist_destroy (&measurements);
    
    //FAIL
    id = NO_DEVICE_ID;
    name = "";
    measurements = select_last_measurements (url.c_str(), id, name);
    CHECK (name == "");
    REQUIRE ( measurements );
    REQUIRE (zlist_size(measurements) == 0 );*/
    zlist_destroy (&measurements);
}

TEST_CASE("helper functions: convert_asset_to_monitor", "[db][convert_to_monitor]")
{
    tntdb::Connection conn;
    REQUIRE_NOTHROW (conn = tntdb::connectCached (url));
    tntdb::Value val;
    uint32_t id_asset = 0;
    uint32_t id_monitor = 0;

    tntdb::Statement st = conn.prepareCached (
       "select id_asset_element from t_bios_asset_element where name = 'DC1'"
    );
    REQUIRE_NOTHROW (val = st.selectValue ());
    REQUIRE_NOTHROW (val.get (id_asset));
    REQUIRE_THROWS_AS(convert_asset_to_monitor (url.c_str(), id_asset), bios::NotFound );

    st = conn.prepareCached(
       "select id_asset_element from t_bios_asset_element where name = 'ROW1'"
    );
    REQUIRE_NOTHROW (val = st.selectValue ());
    REQUIRE_NOTHROW (val.get (id_asset));

    st = conn.prepareCached(
       "select id_asset_element from t_bios_asset_element where name = 'ups'"
    );
    REQUIRE_NOTHROW (val = st.selectValue ());
    REQUIRE_NOTHROW (val.get (id_asset));

    st = conn.prepareCached (
       "select id_discovered_device from t_bios_discovered_device "
       "where name = 'select_device' AND id_device_type = "
       "(select id_device_type from t_bios_device_type where name = 'not_classified')"
    );
    REQUIRE_NOTHROW (val = st.selectValue ());
    REQUIRE_NOTHROW (val.get (id_monitor));

    REQUIRE ( convert_asset_to_monitor (url.c_str(), id_asset) == id_monitor);
}

TEST_CASE("helper functions: convert_monitor_to_asset", "[db][convert_to_asset]")
{
    tntdb::Connection conn;
    REQUIRE_NOTHROW (conn = tntdb::connectCached(url));
    tntdb::Value val;
    uint32_t id_asset = 0;
    uint32_t id_monitor = 0;

    tntdb::Statement st = conn.prepareCached (
       "select id_discovered_device from t_bios_discovered_device "
       "where name = 'select_device' AND id_device_type = "
       "(select id_device_type from t_bios_device_type where name = 'not_classified')"
    );
    REQUIRE_NOTHROW (val = st.selectValue ());
    REQUIRE_NOTHROW (val.get (id_monitor));

    st = conn.prepareCached (
       "select id_asset_element from t_bios_asset_element where name = 'ups'"
    );
    REQUIRE_NOTHROW (val = st.selectValue ());
    REQUIRE_NOTHROW (val.get (id_asset));
    REQUIRE ( convert_monitor_to_asset (url.c_str(), id_monitor) == id_asset );

    id_monitor = 65530;
    REQUIRE_THROWS_AS ( convert_monitor_to_asset (url.c_str(), id_monitor),  bios::NotFound );
}


TEST_CASE("get_last_measurements", "[db][get][lastmeasurements]")
{
    //TODO
    //SUCCESS
 /*   tntdb::Connection conn;
    REQUIRE_NOTHROW (conn = tntdb::connectCached(url));
    tntdb::Value val;
    uint32_t id = 0;

    tntdb::Statement st = conn.prepareCached (
       "select id_asset_element from t_bios_asset_element where name = 'ups'"
    );
    REQUIRE_NOTHROW (val = st.selectValue ());
    REQUIRE_NOTHROW (val.get (id));

    zmsg_t* getlastmeasurements = common_msg_encode_get_last_measurements (id);
    common_msg_t* glm = common_msg_decode (&getlastmeasurements);
    common_msg_print (glm);
    zmsg_t* returnmeasurements = _get_last_measurements (url.c_str(), glm);
    common_msg_print (glm);
    REQUIRE ( returnmeasurements );

    REQUIRE ( is_common_msg (returnmeasurements) );
    common_msg_destroy (&glm);

    glm = common_msg_decode (&returnmeasurements);
    REQUIRE ( common_msg_id (glm) == COMMON_MSG_RETURN_LAST_MEASUREMENTS );
    REQUIRE ( common_msg_device_id (glm) == id );
    zlist_t* measurements = common_msg_get_measurements (glm);
    
    REQUIRE (zlist_size(measurements) == EXP.size());
    std::set<std::string> results;
    for (char* s = (char*) zlist_first(measurements); s != NULL; s = (char*) zlist_next(measurements)) {
        results.insert(s);
    }
    REQUIRE (results == EXP);
    
    zlist_destroy (&measurements);
    common_msg_destroy (&glm);

    //FAIL
    id = 65531;
    getlastmeasurements = common_msg_encode_get_last_measurements (id);
    glm = common_msg_decode (&getlastmeasurements);
    returnmeasurements = _get_last_measurements (url.c_str(), glm);
    REQUIRE ( returnmeasurements );
    REQUIRE ( is_common_msg (returnmeasurements) );
    common_msg_destroy (&glm);

    glm = common_msg_decode (&returnmeasurements);
    REQUIRE ( common_msg_id (glm) == COMMON_MSG_FAIL );
    REQUIRE ( common_msg_errorno (glm) == DB_ERROR_NOTFOUND );
    common_msg_destroy (&glm);
*/
}
