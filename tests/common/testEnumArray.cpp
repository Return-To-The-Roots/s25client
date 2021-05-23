// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "helpers/EnumArray.h"
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <array>

namespace {
enum class TestEnum
{
    Value0,
    Value1,
    Value2,
    Value3,
    Value4,
};
constexpr auto maxEnumValue(TestEnum)
{
    return TestEnum::Value4;
}

enum class TestEnumSmall
{
    Entry
};
constexpr auto maxEnumValue(TestEnumSmall)
{
    return TestEnumSmall::Entry;
}

} // namespace

BOOST_AUTO_TEST_SUITE(EnumArraySuite)

BOOST_AUTO_TEST_CASE(SizeAndEmpty)
{
    const helpers::EnumArray<int, TestEnum> values{};
    const helpers::EnumArray<int, TestEnumSmall> values2{};
    BOOST_TEST(values.size() == 5u);
    BOOST_TEST(!values.empty());
    BOOST_TEST(values2.size() == 1u);
    BOOST_TEST(!values2.empty());
}

BOOST_AUTO_TEST_CASE(ConvertedValuesAreCorrect)
{
    const std::array<int, 5> expectedValues = {-1, 7, -9, 3, 42};
    const helpers::EnumArray<int, TestEnum> values = helpers::toEnumArray<TestEnum>(expectedValues);
    BOOST_TEST(values[TestEnum::Value0] == expectedValues[0]);
    BOOST_TEST(values[TestEnum::Value1] == expectedValues[1]);
    BOOST_TEST(values[TestEnum::Value2] == expectedValues[2]);
    BOOST_TEST(values[TestEnum::Value3] == expectedValues[3]);
    BOOST_TEST(values[TestEnum::Value4] == expectedValues[4]);
    BOOST_TEST(values == expectedValues, boost::test_tools::per_element{});
    static_assert(values.size() == expectedValues.size(), "!");
    BOOST_TEST(std::equal(values.begin(), values.end(), expectedValues.begin(), expectedValues.end()));
}

BOOST_AUTO_TEST_SUITE_END()
