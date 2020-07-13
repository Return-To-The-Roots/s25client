// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "Settings.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(SettingsSuite)

BOOST_AUTO_TEST_CASE(CheckPort)
{
    BOOST_REQUIRE(!validate::checkPort("-1"));
    BOOST_REQUIRE(!validate::checkPort("0"));
    BOOST_REQUIRE(!validate::checkPort("65536"));
    BOOST_REQUIRE(!validate::checkPort("-1"));
    BOOST_REQUIRE(!validate::checkPort("1-6"));
    BOOST_REQUIRE(!validate::checkPort("1.1"));
    boost::optional<uint16_t> port = validate::checkPort("1");
    BOOST_REQUIRE(port);
    BOOST_REQUIRE_EQUAL(*port, 1u);
    port = validate::checkPort("100");
    BOOST_REQUIRE(port);
    BOOST_REQUIRE_EQUAL(*port, 100u);
    port = validate::checkPort("65535");
    BOOST_REQUIRE(port);
    BOOST_REQUIRE_EQUAL(*port, 65535u);
}

BOOST_AUTO_TEST_SUITE_END()
