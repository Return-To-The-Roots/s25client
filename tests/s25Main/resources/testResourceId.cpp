// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "resources/ResourceId.h"
#include <boost/filesystem/path.hpp>
#include <boost/test/unit_test.hpp>
#include <cstring>
#include <stdexcept>
#include <type_traits>

namespace fs = boost::filesystem;

BOOST_AUTO_TEST_SUITE(ResourceIdSuite)

BOOST_AUTO_TEST_CASE(ConstructibleFromCStringAndArray)
{
    ResourceId resId1("res_id");
    // Implicitely convertible
    ResourceId resId2 = "res_id";
    BOOST_TEST(resId1 == resId2);
    static_assert(!std::is_convertible_v<ResourceId, const char*>, "Not convertible from char array");
}

BOOST_AUTO_TEST_CASE(ConstructibleFromStdString)
{
    const std::string name = "res_id";
    ResourceId resId1(name);
    BOOST_TEST(resId1 == ResourceId("res_id"));
    static_assert(!std::is_convertible_v<ResourceId, std::string>, "Not convertible from std::string");
}

BOOST_AUTO_TEST_CASE(CopyableAndMovable)
{
    ResourceId resId = "res_id";
    ResourceId resIdCpCtor(resId);
    ResourceId resIdCpAssign = resId;
    BOOST_TEST(resId == resIdCpCtor);
    BOOST_TEST(resId == resIdCpAssign);
    ResourceId resIdMvCtor(std::move(resIdCpCtor));      // NOLINT(performance-move-const-arg)
    ResourceId resIdMVAssign = std::move(resIdCpAssign); // NOLINT(performance-move-const-arg)
    BOOST_TEST(resId == resIdMvCtor);
    BOOST_TEST(resId == resIdMVAssign);
}

BOOST_AUTO_TEST_CASE(Comparable)
{
    BOOST_TEST(ResourceId("foo") == ResourceId("foo"));
    BOOST_TEST(ResourceId("foo") != ResourceId("bar"));
    BOOST_TEST(ResourceId("foo") != ResourceId("foo2"));
    // For usage in maps
    BOOST_TEST(ResourceId("abc") < ResourceId("def"));
    BOOST_TEST(ResourceId("abc") < ResourceId("abcd"));
}

// All constexpr
BOOST_AUTO_TEST_CASE(AllConstexpr)
{
    // Construct
    constexpr ResourceId resId1("foo");
    constexpr ResourceId resId2 = "foo";
    constexpr ResourceId resId3("bar");
    static_assert(resId1 == resId2, "Equals");
    static_assert(resId1 != resId3, "Not equals");
    static_assert(resId3 < resId1, "Less than");
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

#ifndef NDEBUG
BOOST_AUTO_TEST_CASE(ConstructorDetectsInvalidInNDEBUG_Mode)
{
    // Only alpha-numeric chars and underscore are allowed. Max length of 15
    BOOST_CHECK_NO_THROW(ResourceId("abyz091_"));
    BOOST_CHECK_NO_THROW(ResourceId("123456789012345"));
    BOOST_CHECK_NO_THROW(ResourceId("123456789012345"));
    BOOST_CHECK_THROW(ResourceId("$ab"), std::logic_error);
    BOOST_CHECK_THROW(ResourceId("a$b"), std::logic_error);
    BOOST_CHECK_THROW(ResourceId("ab$"), std::logic_error);
    BOOST_CHECK_THROW(ResourceId("$"), std::logic_error);
    // Constructing an overlong name directly is caught at compiletime. Check string here
    BOOST_CHECK_THROW(ResourceId(std::string("1234567890123456")), std::logic_error);
    // Must not be empty
    BOOST_CHECK_THROW(ResourceId(""), std::logic_error);
}
#endif

BOOST_AUTO_TEST_SUITE_END()
