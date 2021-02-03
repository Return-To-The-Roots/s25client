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

#include "helpers/OptionalEnum.h"
#include "helpers/OptionalIO.h"
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>

namespace testEnums {
enum class DefaultEnum
{
    Value1,
    Value2
};
enum class SignedEnum : signed char
{
    Value1,
    Value2
};
enum class UnsignedEnum : unsigned char
{
    Value1,
    Value2
};

// LCOV_EXCL_START
template<typename T>
static std::enable_if_t<std::is_enum<T>::value, std::ostream&> operator<<(std::ostream& os, T enumVal)
{
    return os << static_cast<int>(enumVal);
}
// LCOV_EXCL_STOP

constexpr auto maxEnumValue(DefaultEnum)
{
    return DefaultEnum::Value2;
}
constexpr auto maxEnumValue(SignedEnum)
{
    return SignedEnum::Value2;
}
constexpr auto maxEnumValue(UnsignedEnum)
{
    return UnsignedEnum::Value2;
}

} // namespace testEnums

using namespace testEnums;

using EnumsToTest = boost::mpl::list<DefaultEnum, SignedEnum, UnsignedEnum>;

BOOST_AUTO_TEST_SUITE(OptionalEnum)

BOOST_AUTO_TEST_CASE_TEMPLATE(EmptyContainsNoValue, T, EnumsToTest)
{
    helpers::OptionalEnum<T> defConstruct;
    BOOST_TEST(!defConstruct);
    BOOST_TEST(!defConstruct.has_value());
    BOOST_CHECK_THROW(defConstruct.value(), boost::bad_optional_access);
    BOOST_TEST(defConstruct.value_or(T::Value1) == T::Value1);
    BOOST_TEST(defConstruct.value_or(T::Value2) == T::Value2);

    helpers::OptionalEnum<T> noneConstruct(boost::none);
    BOOST_TEST(!noneConstruct);
    BOOST_TEST(!noneConstruct.has_value());
    BOOST_CHECK_THROW(noneConstruct.value(), boost::bad_optional_access);
    BOOST_TEST(noneConstruct.value_or(T::Value1) == T::Value1);
    BOOST_TEST(noneConstruct.value_or(T::Value2) == T::Value2);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(ContainsValue, T, EnumsToTest)
{
    for(const auto val : {T::Value1, T::Value2})
    {
        helpers::OptionalEnum<T> construct(val);
        BOOST_TEST(construct);
        BOOST_TEST(construct.has_value());
        BOOST_TEST(construct.value() == val);
        BOOST_TEST(construct.value_or(T::Value1) == val);

        helpers::OptionalEnum<T> assign;
        assign = val;
        BOOST_TEST(assign);
        BOOST_TEST(assign.has_value());
        BOOST_TEST(assign.value() == val);
        BOOST_TEST(assign.value_or(T::Value1) == val);
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(NoValueAfterReset, T, EnumsToTest)
{
    for(const auto val : {T::Value1, T::Value2})
    {
        helpers::OptionalEnum<T> optVal = val; // Deliberately test = construct
        BOOST_TEST(optVal.has_value());
        BOOST_TEST(optVal.value() == val);

        optVal.reset();
        BOOST_TEST(!optVal.has_value());
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(Compare, T, EnumsToTest)
{
    helpers::OptionalEnum<T> optVal1(T::Value1);
    helpers::OptionalEnum<T> optVal1_2(T::Value1);
    helpers::OptionalEnum<T> optVal2(T::Value2);
    helpers::OptionalEnum<T> optValEmpty;
    helpers::OptionalEnum<T> optValEmpty2;
    BOOST_TEST(optVal1 == optVal1_2);
    BOOST_TEST(optVal1 != optVal2);
    BOOST_TEST(optVal1 != optValEmpty);
    BOOST_TEST(optVal2 != optValEmpty);
    BOOST_TEST(optValEmpty == optValEmpty2);

    // Against value, both sides
    BOOST_TEST(optVal1 == T::Value1);
    BOOST_TEST(T::Value1 == optVal1);
    BOOST_TEST(T::Value1 != optVal2);
    BOOST_TEST(optVal2 != T::Value1);
    BOOST_TEST(T::Value1 != optValEmpty);
    BOOST_TEST(T::Value2 != optValEmpty);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(CopyAndMove, T, EnumsToTest)
{
    helpers::OptionalEnum<T> optVal1(T::Value1);
    helpers::OptionalEnum<T> optVal2(T::Value2);

    helpers::OptionalEnum<T> optVal2_2(optVal2);
    optVal1 = optVal2;
    BOOST_TEST(optVal2_2 == T::Value2);
    BOOST_TEST(optVal1 == T::Value2);

    helpers::OptionalEnum<T> optVal1_2(T::Value1);
    helpers::OptionalEnum<T> optVal1_3(std::move(optVal1_2));
    BOOST_TEST(optVal1_3 == T::Value1);
    optVal1 = std::move(optVal1_3);
    BOOST_TEST(optVal1 == T::Value1);
}

BOOST_AUTO_TEST_SUITE_END()
