// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
