// Copyright (c) 2020 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

} // namespace

BOOST_AUTO_TEST_SUITE(EnumArraySuite)

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
