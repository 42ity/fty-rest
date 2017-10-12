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
 * \file test-loadcsv.cc
 * \author Michal Vyskocil <MichalVyskocil@Eaton.com>
 * \author Tomas Halman <TomasHalman@Eaton.com>
 * \author Alena Chernikava <AlenaChernikava@Eaton.com>
 * \brief Not yet documented file
 */
#include <catch.hpp>
#include <fstream>
#include <cxxtools/csvdeserializer.h>
#include <tntdb/result.h>
#include <tntdb/statement.h>

#include "csv.h"
#include "log.h"
#include "db/inout.h"
#include "assetcrud.h"
#include "dbpath.h"

using namespace persist;

std::string get_dc_lab_description() {
    tntdb::Connection conn;
    REQUIRE_NOTHROW ( conn = tntdb::connectCached(url) );

    tntdb::Statement st = conn.prepareCached(
        " SELECT value from t_bios_asset_ext_attributes"
        " WHERE id_asset_element in ("
        "   SELECT id_asset_element FROM t_bios_asset_element"
        "   WHERE NAME = 'DC-LAB')"
        " AND keytag='description'"
    );

    // this is ugly, but was lazy to read the docs
    tntdb::Result result = st.select();
    for ( auto &row: result )
    {
        std::string ret;
        row[0].get(ret);
        return ret;
    }

    return std::string{"could-not-get-here"};  //to make gcc happpy
}

inline std::string to_utf8(const cxxtools::String& ws) {
    return cxxtools::Utf8Codec::encode(ws);
}

TEST_CASE("CSV multiple field names", "[csv]") {

    std::string base_path{__FILE__};
    std::string csv = base_path + ".csv";
    std::string tsv = base_path + ".tsv";
    std::string ssv = base_path + ".ssv";
    std::string usv = base_path + ".usv";
    std::vector <std::pair<db_a_elmnt_t,persist::asset_operation>> okRows;
    std::map <int, std::string> failRows;

    static std::string exp = to_utf8(cxxtools::String(L"Lab DC(тест)"));
    REQUIRE_NOTHROW(get_dc_lab_description() == exp);

    auto void_fn = []() {return;};

    // test coma separated values format
    std::fstream csv_buf{csv};
    REQUIRE_NOTHROW(load_asset_csv(csv_buf, okRows, failRows, void_fn ));

    // test tab separated values
    std::fstream tsv_buf{tsv};
    REQUIRE_NOTHROW(load_asset_csv(tsv_buf, okRows, failRows, void_fn ));

    // test semicolon separated values
    std::fstream ssv_buf{ssv};
    REQUIRE_NOTHROW(load_asset_csv(ssv_buf, okRows, failRows, void_fn ));

    // test underscore separated values
    std::fstream usv_buf{usv};
    REQUIRE_THROWS_AS (load_asset_csv(usv_buf, okRows, failRows, void_fn ), std::invalid_argument);
}

TEST_CASE("CSV bug 661 - segfault with quote in name", "[csv]") {

    std::string base_path{__FILE__};
    std::string csv = base_path + ".661.csv";
    std::vector <std::pair<db_a_elmnt_t,persist::asset_operation>> okRows;
    std::map <int, std::string> failRows;

    auto void_fn = []() {return;};

    std::fstream csv_buf{csv};
    REQUIRE_THROWS_AS(load_asset_csv(csv_buf, okRows, failRows, void_fn), std::out_of_range);
}

TEST_CASE("CSV bug 661 - segfault on csv without mandatory columns", "[csv]") {

    std::string base_path{__FILE__};
    std::string csv = base_path + ".661.2.csv";
    std::vector <std::pair<db_a_elmnt_t,persist::asset_operation>> okRows;
    std::map <int, std::string> failRows;

    auto void_fn = []() {return;};
    std::fstream csv_buf{csv};
    REQUIRE_THROWS_AS(load_asset_csv(csv_buf, okRows, failRows, void_fn), std::invalid_argument);
}

TEST_CASE("CSV bug 661 - segfault ...", "[csv]") {

    std::string base_path{__FILE__};
    std::string csv = base_path + ".group3.csv";
    std::vector <std::pair<db_a_elmnt_t,persist::asset_operation>> okRows;
    std::map <int, std::string> failRows;

    auto void_fn = []() {return;};
    std::fstream csv_buf{csv};
    REQUIRE_NOTHROW(load_asset_csv(csv_buf, okRows, failRows, void_fn));
}

class MlmClientTest : MlmClient {
    public:
        MlmClientTest(int s1, int s2, int srv) {
            s1_state = s1;
            s2_state = s2;
            sendto_rv = srv;
            request = 0;
        };
        ~MlmClientTest() {};
        zmsg_t* recv (const std::string& uuid, uint32_t timeout) {
            switch (request) {
                case 1:
                    zmsg_t *resp = zmsg_new ();
                    switch (s2_state) {
                        case 0:
                            zmsg_pushstr (resp, "OK");      // we don't really need those, but we need to pushstr them
                            zmsg_pushstr (resp, "useless"); // to conform to original message
                            zmsg_pushstr (resp, "useless");
                            zmsg_pushstr (resp, "useless");
                            zmsg_pushstr (resp, "useless");
                            zhash_t *map = zhash_new();
                            zhash_insert(map, "serial", "S3R1ALN0");
                            zhash_insert(map, "hostname", "myhname");
                            zhash_insert(map, "ip.1", "98.76.54.30");
                            zhash_insert(map, "ip.2", "98.76.54.31");
                            //zhash_insert(map, "ip.3", "98.76.54.32");
                            zframe_t * frame_infos = zhash_pack(map);
                            zmsg_append (resp, &frame_infos);
                            break;
                        default:
                            zmsg_pushstr (resp, "ERROR");
                            zmsg_pushstr (resp, "just error");
                            break;
                    }
                    return resp;
                    break;
                case 2:
                    zmsg_t *reply = zmsg_new ();
                    switch (s2_state) {
                        case 0:
                            zmsg_addstr (reply, "OK");
                            break;
                        case 1:
                            zmsg_addstr (reply, "OK");
                            zmsg_addstr (reply, "rackcontroller-0");
                            break;
                        case 2:
                            zmsg_addstr (reply, "OK");
                            zmsg_addstr (reply, "rackcontroller-0");
                            zmsg_addstr (reply, "rackcontroller-10");
                            zmsg_addstr (reply, "rackcontroller-99");
                            break;
                        default:
                            zmsg_addstr (reply, "ERROR");
                            zmsg_addstr (reply, "just error");
                            break;
                    }
                    return reply;
                    break;
                default:
                    break;
                    zsys_error("MlmClientTest-recv:Receieved invalid request %d", request);
                    return NULL;
            }
        }
        int sendto (const std::string& address, const std::string& subject, uint32_t timeout, zmsg_t **content_p) {
            if ("info" == subject && "fty-info" == address) {
                request = 1;
            }
            if ("ASSETS" == subject && "asset-agent" == address) {
                request = 2;
            }
            zmsg_destroy(content_p);
            *content_p = NULL;
            return sendto_rv;
        }
        void set_s1_state(int s) {
            s1_state = s;
        }
        void set_s2_state(int s) {
            s2_state = s;
        }
        void set_sendto_rv(int s) {
            sendto_rv = e;
        }
    private:
        int s1_state;
        int s2_state;
        int sendto_rv;
        int request;
};

TEST_CASE("test for rackcontroller-0 detection", "[csv]") {
    auto void_fn = []() {return;};
    size_t rv;
    size_t row_number;
    
    // empty database, 0 RCs in csv - detect none
    row_number = SIZE_MAX;
    MlmClientTest mct(0, 0, 0);
    std::string base_path{__FILE__};
    std::string csv = base_path + "rc0_1.csv";
    std::fstream csv_buf(csv);
    REQUIRE_NOTHROW(rv = promote_rc0(mct, csv_buf, void_fn));
    REQUIRE(row_number == rv);

    // empty database, 1 RC in csv - promote it - was on row number:
    row_number = 7;
    csv = base_path + "rc0_2.csv";
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = promote_rc0(mct, csv_buf, void_fn));
    REQUIRE(row_number == rv);

    // empty database, 3 RC in csv - promote one of it - was on row number:
    row_number = 9;
    csv = base_path + "rc0_3.csv";
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = promote_rc0(mct, csv_buf, void_fn));
    REQUIRE(row_number == rv);

    mct.set_s2_state(1);
    // 1 RC in database, 0 RCs in csv - detect none
    row_number = SIZE_MAX;
    csv = base_path + "rc0_1.csv";
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = promote_rc0(mct, csv_buf, void_fn));
    REQUIRE(SIZE_MAX == rv);

    // 1 RC in database, 1 RC in csv - promote it - was on row number:
    row_number = 7;
    csv = base_path + "rc0_2.csv";
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = promote_rc0(mct, csv_buf, void_fn));
    REQUIRE(row_number == rv);

    // 1 RC in database, 3 RC in csv - promote one of it - was on row number:
    row_number = 9;
    csv = base_path + "rc0_3.csv";
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = promote_rc0(mct, csv_buf, void_fn));
    REQUIRE(row_number == rv);

    // 1 RC in database, 3 RC in csv - was none of it
    row_number = SIZE_MAX;
    csv = base_path + "rc0_4.csv";
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = promote_rc0(mct, csv_buf, void_fn));
    REQUIRE(row_number == rv);

    // 1 RC in database, 3 RC in csv - was different than rackcontroller-0
    row_number = SIZE_MAX;
    csv = base_path + "rc0_5.csv";
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = promote_rc0(mct, csv_buf, void_fn));
    REQUIRE(row_number == rv);

    mct.set_s2_state(2);
    // 3 RC in database, 0 RCs in csv - detect none
    row_number = SIZE_MAX;
    csv = base_path + "rc0_1.csv";
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = promote_rc0(mct, csv_buf, void_fn));
    REQUIRE(SIZE_MAX == rv);

    // 3 RC in database, 1 RC in csv - promote it - was on row number:
    row_number = 7;
    csv = base_path + "rc0_2.csv";
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = promote_rc0(mct, csv_buf, void_fn));
    REQUIRE(row_number == rv);

    // 3 RC in database, 3 RC in csv - promote one of it - was on row number:
    row_number = 9;
    csv = base_path + "rc0_3.csv";
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = promote_rc0(mct, csv_buf, void_fn));
    REQUIRE(row_number == rv);

    // 3 RC in database, 3 RC in csv - was none of it
    row_number = SIZE_MAX;
    csv = base_path + "rc0_4.csv";
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = promote_rc0(mct, csv_buf, void_fn));
    REQUIRE(row_number == rv);

    // 3 RC in database, 3 RC in csv - was different than rackcontroller-0
    row_number = SIZE_MAX;
    csv = base_path + "rc0_5.csv";
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = promote_rc0(mct, csv_buf, void_fn));
    REQUIRE(row_number == rv);

    // 3 RC in database, 3 RC in csv - with missing ids
    row_number = 9;
    csv = base_path + "rc0_6.csv";
    csv_buf.close();
    csv_buf.clear();
    csv_buf.open(csv);
    REQUIRE_NOTHROW(rv = promote_rc0(mct, csv_buf, void_fn));
    REQUIRE(row_number == rv);

    mct.set_s2_state(99);
    // getting assets from fty_assets fails
    REQUIRE_THROWS_AS(rv = promote_rc0(mct, csv_buf, void_fn), std::runtime_error);

    mct.set_s1_state(99);
    // getting info from fty_info fails
    REQUIRE_THROWS_AS(rv = promote_rc0(mct, csv_buf, void_fn), std::runtime_error);

    mct.set_sendto_rv(-1);
    // sendto fails fails
    REQUIRE_THROWS_AS(rv = promote_rc0(mct, csv_buf, void_fn), std::runtime_error);
}
