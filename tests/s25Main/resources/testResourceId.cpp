// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "resources/ResourceId.h"
#include <boost/test/unit_test.hpp>
#include <stdexcept>

namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE(ResourceIdSuite)

BOOST_AUTO_TEST_CASE(ConstructibleFromStdString)
{
    const std::string name = "res_id";
    ResourceId resId1(name);
    // Implicitely convertible
    ResourceId resId2 = name;
    BOOST_TEST(resId1 == resId2);
}

BOOST_AUTO_TEST_CASE(ConstructibleFromCStringAndArray)
{
    const char* name = "res_id";
    ResourceId resId1(name);
    ResourceId resId3("res_id");
    // Implicitely convertible
    ResourceId resId2 = name;
    ResourceId resId4 = "res_id";
    BOOST_TEST(resId1 == resId2);
    BOOST_TEST(resId3 == resId4);
}

BOOST_AUTO_TEST_CASE(Comparable)
{
    BOOST_TEST(ResourceId("foo") == ResourceId("foo"));
    BOOST_TEST(ResourceId("foo") != ResourceId("bar"));
    // For usage in maps
    BOOST_TEST(ResourceId("abc") < ResourceId("def"));
}

BOOST_AUTO_TEST_CASE(MakeValidFromPath)
{
    // Special case: no change if constructed from other ResourceId
    BOOST_TEST(ResourceId::make(ResourceId("foo")) == ResourceId("foo"));

    BOOST_TEST(ResourceId::make(fs::path("foo")) == ResourceId("foo"));
    // Extensions stripped
    BOOST_TEST(ResourceId::make(fs::path("foo.lst")) == ResourceId("foo"));
    BOOST_TEST(ResourceId::make(fs::path("foo.lst.txt")) == ResourceId("foo"));
    // Path is lower cased
    BOOST_TEST(ResourceId::make(fs::path("FOO.LST")) == ResourceId("foo"));
    // Only filestem matters
    BOOST_TEST(ResourceId::make(fs::path("top") / "level" / "folder" / "foo.lst") == ResourceId("foo"));
}

BOOST_AUTO_TEST_CASE(MakeDetectsInvalid)
{
    // Only alpha-numeric chars and underscore are allowed
    BOOST_TEST(ResourceId::make(fs::path("abyzABYZ091_.txt")) == ResourceId("abyzabyz091_"));
    BOOST_CHECK_THROW(ResourceId::make(fs::path("$ab.txt")), std::invalid_argument);
    BOOST_CHECK_THROW(ResourceId::make(fs::path("a$b.txt")), std::invalid_argument);
    BOOST_CHECK_THROW(ResourceId::make(fs::path("ab$.txt")), std::invalid_argument);
    BOOST_CHECK_THROW(ResourceId::make(fs::path("$.txt")), std::invalid_argument);
    // Must not be empty
    BOOST_CHECK_THROW(ResourceId::make(fs::path("")), std::invalid_argument);
    BOOST_CHECK_THROW(ResourceId::make(fs::path(".txt")), std::invalid_argument);
    BOOST_CHECK_THROW(ResourceId::make(boost::filesystem::path("parent") / ".empty"), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
