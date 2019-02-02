// Copyright (c) 2018 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "commonDefines.h" // IWYU pragma: keep
#include "helpers/strUtils.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(Helpers)

BOOST_AUTO_TEST_CASE(join)
{
    std::vector<std::string> items;
    BOOST_REQUIRE_EQUAL(helpers::join(items, ", "), "");
    BOOST_REQUIRE_EQUAL(helpers::join(items, ", ", " and "), "");
    items.push_back("foo1");
    BOOST_REQUIRE_EQUAL(helpers::join(items, ", "), "foo1");
    BOOST_REQUIRE_EQUAL(helpers::join(items, ", ", " and "), "foo1");
    items.push_back("foo2");
    BOOST_REQUIRE_EQUAL(helpers::join(items, ", "), "foo1, foo2");
    BOOST_REQUIRE_EQUAL(helpers::join(items, ", ", " and "), "foo1 and foo2");
    items.push_back("foo3");
    BOOST_REQUIRE_EQUAL(helpers::join(items, ", "), "foo1, foo2, foo3");
    BOOST_REQUIRE_EQUAL(helpers::join(items, ", ", " and "), "foo1, foo2 and foo3");
    items.push_back("foo4");
    BOOST_REQUIRE_EQUAL(helpers::join(items, ", "), "foo1, foo2, foo3, foo4");
    BOOST_REQUIRE_EQUAL(helpers::join(items, ", ", " and "), "foo1, foo2, foo3 and foo4");
}

BOOST_AUTO_TEST_SUITE_END()
