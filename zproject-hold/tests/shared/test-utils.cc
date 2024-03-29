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
 * \file test-utils.cc
 * \author Karol Hrdina <KarolHrdina@Eaton.com>
 * \author Michal Vyskocil <MichalVyskocil@Eaton.com>
 * \author Alena Chernikava <AlenaChernikava@Eaton.com>
 * \brief Not yet documented file
 */
#include <czmq.h>
#include <catch.hpp>
#include <fty_common.h>

#include "shared/utils.h"

// Note: Tests here do bad things on purpose
// Do not let vigilant compilers complain :)
#ifdef __GNUC__
# pragma GCC diagnostic ignored "-Woverflow"
#endif

TEST_CASE ("average_step_seconds", "[average_step_seconds][utils][average]") {

    SECTION ("bad arguments") {
        CHECK ( average_step_seconds ("15s") == -1 );
        CHECK ( average_step_seconds ("15h") == -1 );
        CHECK ( average_step_seconds ("15d") == -1 );

        CHECK ( average_step_seconds ("30h") == -1 );

        CHECK ( average_step_seconds ("1m") == -1 );
        CHECK ( average_step_seconds ("24m") == -1 );
        CHECK ( average_step_seconds ("24d") == -1 );

        CHECK ( average_step_seconds ("1d") == -1 );
        CHECK ( average_step_seconds ("1440m") == -1 );
        CHECK ( average_step_seconds ("2d") == -1 );
    }
    SECTION ("correct execution") {
        CHECK ( average_step_seconds ("15m") == 900 );
        CHECK ( average_step_seconds ("30m") == 1800 );
        CHECK ( average_step_seconds ("1h") == 3600 );
        CHECK ( average_step_seconds ("24h") == 86400 );
        CHECK ( average_step_seconds ("7d") == 7*24*60*60 );
        CHECK ( average_step_seconds ("30d") == 30*24*60*60 );
    }
}

TEST_CASE ("datetime_to_calendar", "[utils][time]") {
    SECTION ("bad arguments") {
        CHECK ( datetime_to_calendar (NULL) == -1 );
        CHECK ( datetime_to_calendar ("") == -1 );
        CHECK ( datetime_to_calendar ("20150419181523") == -2 ); // missing utc timezone specifier
        CHECK ( datetime_to_calendar ("20150419181523z") == -2 ); // wrong small case
        CHECK ( datetime_to_calendar ("2015041918152Z") == -2 ); // one number short
    }
    SECTION ("correct execution") {
        CHECK ( datetime_to_calendar ("20150301100532Z") == 1425204332 );
        CHECK ( datetime_to_calendar ("20150304235959Z") == 1425513599 );
        CHECK ( datetime_to_calendar ("19991231235959Z") == 946684799 );
        CHECK ( datetime_to_calendar ("20000101000000Z") == 946684800 );
        CHECK ( datetime_to_calendar ("20000131235959Z") == 949363199 );
        CHECK ( datetime_to_calendar ("20000201000000Z") == 949363200 );
        CHECK ( datetime_to_calendar ("20000229235959Z") == 951868799 );
        CHECK ( datetime_to_calendar ("20000301000000Z") == 951868800 );
        CHECK ( datetime_to_calendar ("20000526135504Z") == 959349304 );
        CHECK ( datetime_to_calendar ("20000430235959Z") == 957139199 );
        CHECK ( datetime_to_calendar ("20000501000000Z") == 957139200 );
        CHECK ( datetime_to_calendar ("20001231235959Z") == 978307199 );
        CHECK ( datetime_to_calendar ("20010101000000Z") == 978307200 );
        CHECK ( datetime_to_calendar ("20010731235959Z") == 996623999 );
        CHECK ( datetime_to_calendar ("20010801000000Z") == 996624000 );
        CHECK ( datetime_to_calendar ("20010930235959Z") == 1001894399 );
        CHECK ( datetime_to_calendar ("20011001000000Z") == 1001894400 );
    }
}

TEST_CASE("addi32_overflow", "[utils][overflow]") {
    int32_t a, b, value;
    bool ret;

    ret = addi32_overflow(22, 20, &value);
    CHECK(ret);
    CHECK(value == 42);

    ret = addi32_overflow(INT32_MAX, 0, &value);
    CHECK(ret);
    CHECK(value == INT32_MAX);

    ret = addi32_overflow(INT32_MAX, 2, &value);
    CHECK(!ret);
    CHECK(value == INT32_MAX); // old value

    ret = addi32_overflow(66, -24, &value);
    CHECK(ret);
    CHECK(value == 42);

    ret = addi32_overflow(INT32_MIN, -1, &value);
    CHECK(!ret);
    CHECK(value == 42); // old value
}

TEST_CASE("bsi32_rescale","[utils][bs_rescale]"){

    int32_t value;
    bool ret;

    // upscale
    ret = bsi32_rescale(42, 0, 1, &value);
    CHECK(ret);
    CHECK(value == 4);

    ret = bsi32_rescale(42, 0, 3, &value);
    CHECK(ret);
    CHECK(value == 0);

    // down
    ret = bsi32_rescale(42, 0, -3, &value);
    CHECK(ret);
    CHECK(value == 42000);

    // underflow
    ret = bsi32_rescale(42, 0, -128, &value);
    CHECK(!ret);
    CHECK(value == 42000); //<<< just the previous value

    // overflow
    ret = bsi32_rescale(42, 0, 128, &value);
    CHECK(!ret);
    CHECK(value == 42000); //<<< just the previous value

    // upscale - negative
    ret = bsi32_rescale(-42, 0, 1, &value);
    CHECK(ret);
    CHECK(value == -4);

    // downscale - negative
    ret = bsi32_rescale(-42, 0, -3, &value);
    CHECK(ret);
    CHECK(value == -42000);

    // underflow - negative
    ret = bsi32_rescale(INT32_MIN / 10, 0, -2, &value);
    CHECK(!ret);
    CHECK(value == -42000); //<<< just the previous value
}

TEST_CASE("bsi32_add","[utils][bs_add]"){

    int32_t value;
    int8_t scale;
    bool ret;

    // add with the same scale is easy ...
    ret = bsi32_add(22, 0, 20, 0, &value, &scale);
    CHECK(ret);
    CHECK(value == 42);
    CHECK(scale == 0);

    // ... but we do support any arbitrary scale
    ret = bsi32_add(40, 0, 20, -1, &value, &scale);
    CHECK(ret);
    CHECK(value == 420);
    CHECK(scale == -1);

    // ... and we check overflows
    ret = bsi32_add(40, 0, 20, -128, &value, &scale);
    CHECK(!ret);
    CHECK(value == 420);    //<<< just the previous value
    CHECK(scale == -1);     //<<< just the previous value

    // ... and we check overflows
    ret = bsi32_add(INT32_MAX, 0, 42, 0, &value, &scale);
    CHECK(!ret);
    CHECK(value == 420);    //<<< just the previous value
    CHECK(scale == -1);     //<<< just the previous value

    // negative numbers
    ret = bsi32_add(64, 0, -22, 0, &value, &scale);
    CHECK(ret);
    CHECK(value == 42);
    CHECK(scale == 0);

    // negative numbers - underflows
    ret = bsi32_add(INT32_MIN, 0, -22, 0, &value, &scale);
    CHECK(!ret);
    CHECK(value == 42);    //<<< just the previous value
    CHECK(scale == 0);     //<<< just the previous value
    return;
}

TEST_CASE("sanitize_date", "[utils][sanitize_date]") {

    // sanitize_date is locale specific, but the test expects C locale
    ::setlocale (LC_ALL, "C");

    // invalid date
    char *r = sanitize_date ("123");
    CHECK (r == NULL);

    // ISO date
    r = sanitize_date ("2010-02-15");
    CHECK (r != NULL);
    CHECK (streq (r, "2010-02-15"));
    zstr_free (&r);

    // ISO date w/0 zeros
    r = sanitize_date ("2010-2-15");
    CHECK (r != NULL);
    CHECK (streq (r, "2010-02-15"));
    zstr_free (&r);

    // Excell date
    r = sanitize_date ("15-Feb-10");
    CHECK (r != NULL);
    CHECK (streq (r, "2010-02-15"));
    zstr_free (&r);

    // European date
    r = sanitize_date ("15.02.2010");
    CHECK (r != NULL);
    CHECK (streq (r, "2010-02-15"));
    zstr_free (&r);

    // ISO date
    r = sanitize_date ("2010-2-2");
    CHECK (r != NULL);
    CHECK (streq (r, "2010-02-02"));
    zstr_free (&r);

    // European date
    r = sanitize_date ("15 02 2010");
    CHECK (r != NULL);
    CHECK (streq (r, "2010-02-15"));
    zstr_free (&r);

    // European date
    r = sanitize_date ("15 2 2010");
    CHECK (r != NULL);
    CHECK (streq (r, "2010-02-15"));
    zstr_free (&r);

    // US date
    r = sanitize_date ("02/15/2010");
    CHECK (r != NULL);
    CHECK (streq (r, "2010-02-15"));
    zstr_free (&r);

    // US date
    r = sanitize_date ("2/15/2010");
    CHECK (r != NULL);
    CHECK (streq (r, "2010-02-15"));
    zstr_free (&r);

    // Date provided by Yves
    r = sanitize_date ("20-09-2016");
    CHECK (r != NULL);
    CHECK (streq (r, "2016-09-20"));

    //reject date where we can't disntiguish order
    r = sanitize_date ("20-09-16");
    CHECK (r == NULL);

}
