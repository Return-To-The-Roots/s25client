// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "helpers/EnumRange.h"
#include <boost/test/unit_test.hpp>

enum PlainEnum
{
    First,
    Second,
    Third
};
constexpr auto maxEnumValue(PlainEnum)
{
    return PlainEnum::Third;
}

enum class IntEnum : int
{
    First,
    Second,
    Third,
    Forth
};
constexpr auto maxEnumValue(IntEnum)
{
    return IntEnum::Forth;
}

enum class UnsignedEnum : unsigned
{
    First,
    Second
};
constexpr auto maxEnumValue(UnsignedEnum)
{
    return UnsignedEnum::Second;
}

BOOST_AUTO_TEST_CASE(PlainEnumWorks)
{
    std::vector<PlainEnum> result, expected;
    expected = {First, Second, Third};
    for(const PlainEnum e : helpers::enumRange<PlainEnum>())
        result.push_back(e);
    BOOST_TEST(result == expected);
}

BOOST_AUTO_TEST_CASE(IntEnumWorks)
{
    std::vector<IntEnum> result, expected;
    expected = {IntEnum::First, IntEnum::Second, IntEnum::Third, IntEnum::Forth};
    for(const IntEnum e : helpers::enumRange<IntEnum>())
        result.push_back(e);
    BOOST_TEST(result == expected);
}

BOOST_AUTO_TEST_CASE(UnsignedEnumWorks)
{
    std::vector<UnsignedEnum> result, expected;
    expected = {UnsignedEnum::First, UnsignedEnum::Second};
    for(const UnsignedEnum e : helpers::enumRange<UnsignedEnum>())
        result.push_back(e);
    BOOST_TEST(result == expected);
}

BOOST_AUTO_TEST_CASE(OffsetWorks)
{
    std::vector<IntEnum> result, expected;
    expected = {IntEnum::First, IntEnum::Second, IntEnum::Third, IntEnum::Forth};
    for(const IntEnum e : helpers::enumRange(IntEnum::First))
        result.push_back(e);
    BOOST_TEST(result == expected);

    expected = {IntEnum::Second, IntEnum::Third, IntEnum::Forth, IntEnum::First};
    result.clear();
    for(const IntEnum e : helpers::enumRange(IntEnum::Second))
        result.push_back(e);
    BOOST_TEST(result == expected);

    expected = {IntEnum::Third, IntEnum::Forth, IntEnum::First, IntEnum::Second};
    result.clear();
    for(const IntEnum e : helpers::enumRange(IntEnum::Third))
        result.push_back(e);
    BOOST_TEST(result == expected);

    expected = {IntEnum::Forth, IntEnum::First, IntEnum::Second, IntEnum::Third};
    result.clear();
    for(const IntEnum e : helpers::enumRange(IntEnum::Forth))
        result.push_back(e);
    BOOST_TEST(result == expected);
}