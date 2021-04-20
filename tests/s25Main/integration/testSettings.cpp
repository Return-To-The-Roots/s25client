// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Settings.h"
#include <boost/optional/optional_io.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(SettingsSuite)

BOOST_AUTO_TEST_CASE(CheckPort)
{
    BOOST_TEST_REQUIRE(!validate::checkPort("-1"));
    BOOST_TEST_REQUIRE(!validate::checkPort("0"));
    BOOST_TEST_REQUIRE(!validate::checkPort("65536"));
    BOOST_TEST_REQUIRE(!validate::checkPort("-1"));
    BOOST_TEST_REQUIRE(!validate::checkPort("1-6"));
    BOOST_TEST_REQUIRE(!validate::checkPort("1.1"));
    boost::optional<uint16_t> port = validate::checkPort("1");
    BOOST_TEST_REQUIRE(port);
    BOOST_TEST_REQUIRE(*port == 1u);
    port = validate::checkPort("100");
    BOOST_TEST_REQUIRE(port);
    BOOST_TEST_REQUIRE(*port == 100u);
    port = validate::checkPort("65535");
    BOOST_TEST_REQUIRE(port);
    BOOST_TEST_REQUIRE(*port == 65535u);
}

BOOST_AUTO_TEST_SUITE_END()
