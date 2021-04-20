// Copyright (c) 2018 - 2018 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "helpers/strUtils.h"
#include "helpers/toString.h"
#include <boost/test/unit_test.hpp>

namespace {
enum class IntEnum : int16_t
{
    MinusOne = -1,
    Zero = 0,
    PlusTwo = 2
};
}

BOOST_AUTO_TEST_SUITE(Helpers)

BOOST_AUTO_TEST_CASE(toString)
{
    BOOST_TEST(helpers::toString(uint16_t(1)) == "1");
    BOOST_TEST(helpers::toString(uint32_t(65536)) == "65536");
    BOOST_TEST(helpers::toString(int16_t(-1)) == "-1");
    BOOST_TEST(helpers::toString(int32_t(-65536)) == "-65536");
    BOOST_TEST(helpers::toString(IntEnum::MinusOne) == "-1");
    BOOST_TEST(helpers::toString(IntEnum::Zero) == "0");
    BOOST_TEST(helpers::toString(IntEnum::PlusTwo) == "2");
}

BOOST_AUTO_TEST_CASE(join)
{
    std::vector<std::string> items;
    BOOST_TEST(helpers::join(items, ", ") == "");
    BOOST_TEST(helpers::join(items, ", ", " and ") == "");
    items.push_back("foo1");
    BOOST_TEST(helpers::join(items, ", ") == "foo1");
    BOOST_TEST(helpers::join(items, ", ", " and ") == "foo1");
    items.push_back("foo2");
    BOOST_TEST(helpers::join(items, ", ") == "foo1, foo2");
    BOOST_TEST(helpers::join(items, ", ", " and ") == "foo1 and foo2");
    items.push_back("foo3");
    BOOST_TEST(helpers::join(items, ", ") == "foo1, foo2, foo3");
    BOOST_TEST(helpers::join(items, ", ", " and ") == "foo1, foo2 and foo3");
    items.push_back("foo4");
    BOOST_TEST(helpers::join(items, ", ") == "foo1, foo2, foo3, foo4");
    BOOST_TEST(helpers::join(items, ", ", " and ") == "foo1, foo2, foo3 and foo4");
}

BOOST_AUTO_TEST_CASE(concat)
{
    BOOST_TEST(helpers::concat() == "");
    BOOST_TEST(helpers::concat("foo") == "foo");
    BOOST_TEST(helpers::concat(42) == "42");
    BOOST_TEST(helpers::concat(42, "bar") == "42bar");
    BOOST_TEST(helpers::concat(42, "bar", -1337) == "42bar-1337");
}

BOOST_AUTO_TEST_SUITE_END()
