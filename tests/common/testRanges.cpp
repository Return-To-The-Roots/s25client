// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "helpers/EnumRange.h"
#include "helpers/IdRange.h"
#include "helpers/Range.h"
#include "helpers/StrongIdVector.h"
#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(EnumRangeTests)
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

BOOST_AUTO_TEST_CASE(CanAssignToRange)
{
    const auto enums = helpers::EnumRange<IntEnum>{};
    std::vector<IntEnum> enums_vec(enums.begin(), enums.end());
    std::vector<IntEnum> expected{IntEnum::First, IntEnum::Second, IntEnum::Third, IntEnum::Forth};
    BOOST_TEST(enums_vec == expected);

    expected.push_back(expected.front());
    expected.erase(expected.begin());
    const auto enums2 = helpers::enumRange(IntEnum::Second);
    enums_vec = std::vector<IntEnum>(enums2.begin(), enums2.end());
    BOOST_TEST(enums_vec == expected);
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_CASE(RangeClassIsPythonLike)
{
    using IntVec = std::vector<int>;

    // Range-based-for support
    IntVec actual;
    for(const auto i : helpers::range(3))
        actual.push_back(i);
    BOOST_TEST(actual == (IntVec{0, 1, 2}));
    actual.clear();
    for(const auto i : helpers::range(2, 6))
        actual.push_back(i);
    BOOST_TEST(actual == (IntVec{2, 3, 4, 5}));

    // Regular range with end value
    {
        const auto r = helpers::range(5);
        BOOST_TEST(IntVec(r.begin(), r.end()) == (IntVec{0, 1, 2, 3, 4}));
    }
    {
        const auto r = helpers::range(0);
        BOOST_TEST(IntVec(r.begin(), r.end()) == IntVec{});
    }
    // Regular range with start & end value
    {
        const auto r = helpers::range(1, 4);
        BOOST_TEST(IntVec(r.begin(), r.end()) == (IntVec{1, 2, 3}));
    }
    {
        const auto r = helpers::range(4, 4);
        BOOST_TEST(IntVec(r.begin(), r.end()) == IntVec{});
    }
}

BOOST_AUTO_TEST_CASE(IdRangeWorks)
{
    using MyId = helpers::StrongId<unsigned, struct MyIdTag>;
    using Vec = std::vector<MyId>;

    // Range-based-for support
    Vec actual;
    for(const auto i : helpers::idRange<MyId>(3))
        actual.push_back(i);
    BOOST_TEST_REQUIRE(actual.size() == 3u);
    BOOST_TEST(actual[0] == MyId(1));
    BOOST_TEST(actual[1] == MyId(2));
    BOOST_TEST(actual[2] == MyId(3));

    actual.clear();
    for(const auto i : helpers::idRangeAfter(MyId(2), 4))
        actual.push_back(i);
    BOOST_TEST_REQUIRE(actual.size() == 2u);
    BOOST_TEST(actual[0] == MyId(3));
    BOOST_TEST(actual[1] == MyId(4));

    actual.clear();
    for(const auto i : helpers::idRangeAfter(MyId(4), 4))
        actual.push_back(i); // LCOV_EXCL_LINE
    BOOST_TEST(actual.empty());
}

BOOST_AUTO_TEST_CASE(VectorWithStrongId)
{
    using MyId = helpers::StrongId<unsigned, struct MyIdTag>;
    helpers::StrongIdVector<int, MyId> vec(3);
    int ctr = 0;
    for(int& i : vec)
        i = ++ctr;
    ctr = 0;
    for(const auto i : helpers::idRange<MyId>(vec.size()))
        BOOST_TEST(vec[i] == ++ctr);
}
