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

#include "helpers/EnumArray.h"
#include "helpers/MaxEnumValue.h"
#include "helpers/serializeContainers.h"
#include <rttr/test/random.hpp>
#include <s25util/Serializer.h>
#include <boost/test/unit_test.hpp>
#include <utility>

using boost::test_tools::per_element;

namespace {
enum class TestEnum : uint8_t
{
    Value1,
    Value2,
    Value3,
    Value4,
};
constexpr auto maxEnumValue(TestEnum)
{
    return TestEnum::Value4;
}
// LCOV_EXCL_START
std::ostream& operator<<(std::ostream& os, TestEnum e)
{
    return os << static_cast<unsigned>(e);
}
} // namespace

BOOST_AUTO_TEST_SUITE(Serialization)

using ResizableContainers = std::tuple<std::vector<int>, std::vector<uint8_t>, std::list<int>, std::list<uint8_t>>;
using ResizableBoolContainers = std::tuple<std::vector<bool>, std::list<bool>>;
using ResizableEnumContainers = std::tuple<std::vector<TestEnum>, std::list<TestEnum>>;

BOOST_AUTO_TEST_CASE_TEMPLATE(SerializeResizable, T, ResizableContainers)
{
    T empty;
    T single = {rttr::test::randomValue<typename T::value_type>()};
    T many = {13, 17, 23, 27, 31};
    Serializer ser;
    helpers::pushContainer(ser, empty);
    helpers::pushContainer(ser, single);
    helpers::pushContainer(ser, many);

    BOOST_TEST_REQUIRE(helpers::popContainer<T>(ser) == empty, per_element());
    BOOST_TEST_REQUIRE(helpers::popContainer<T>(ser) == single, per_element());
    BOOST_TEST_REQUIRE(helpers::popContainer<T>(ser) == many, per_element());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(SerializeResizableBool, T, ResizableBoolContainers)
{
    T empty;
    T single = {rttr::test::randomBool()};
    T many = {true, false, false, true, true, false};
    Serializer ser;
    helpers::pushContainer(ser, empty);
    helpers::pushContainer(ser, single);
    helpers::pushContainer(ser, many);

    BOOST_TEST_REQUIRE(helpers::popContainer<T>(ser) == empty, per_element());
    BOOST_TEST_REQUIRE(helpers::popContainer<T>(ser) == single, per_element());
    BOOST_TEST_REQUIRE(helpers::popContainer<T>(ser) == many, per_element());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(SerializeResizableEnum, T, ResizableEnumContainers)
{
    T empty;
    T single = {TestEnum(rttr::test::randomValue(0u, helpers::MaxEnumValue_v<TestEnum>))};
    T many = {
      TestEnum::Value1, TestEnum::Value4, TestEnum::Value3, TestEnum::Value2, TestEnum::Value1,
    };
    Serializer ser;
    helpers::pushContainer(ser, empty);
    helpers::pushContainer(ser, single);
    helpers::pushContainer(ser, many);

    BOOST_TEST_REQUIRE(helpers::popContainer<T>(ser) == empty, per_element());
    BOOST_TEST_REQUIRE(helpers::popContainer<T>(ser) == single, per_element());
    BOOST_TEST_REQUIRE(helpers::popContainer<T>(ser) == many, per_element());
}

BOOST_AUTO_TEST_CASE(SerializeFixedSizeContainers)
{
    using rttr::test::randomBool;
    using rttr::test::randomValue;

    std::array<int, 3> intArray = {randomValue<int>(), randomValue<int>(), randomValue<int>()};
    std::array<uint8_t, 4> u8Array = {randomValue<uint8_t>(), randomValue<uint8_t>(), randomValue<uint8_t>(),
                                      randomValue<uint8_t>()};
    std::array<bool, 6> boolArray = {randomBool(), randomBool(), randomBool(),
                                     randomBool(), randomBool(), randomBool()};
    std::array<TestEnum, 3> enumArray = {TestEnum(randomValue(0u, helpers::MaxEnumValue_v<TestEnum>)), TestEnum::Value1,
                                         TestEnum::Value2};
    helpers::EnumArray<int, TestEnum> intEnumArray = {randomValue<int>(), randomValue<int>(), randomValue<int>(),
                                                      randomValue<int>()};

    Serializer ser;
    helpers::pushContainer(ser, intArray);
    helpers::pushContainer(ser, u8Array);
    helpers::pushContainer(ser, boolArray);
    helpers::pushContainer(ser, enumArray);
    helpers::pushContainer(ser, intEnumArray);

    BOOST_TEST_REQUIRE(helpers::popContainer<decltype(intArray)>(ser) == intArray, per_element());
    BOOST_TEST_REQUIRE(helpers::popContainer<decltype(u8Array)>(ser) == u8Array, per_element());
    BOOST_TEST_REQUIRE(helpers::popContainer<decltype(boolArray)>(ser) == boolArray, per_element());
    BOOST_TEST_REQUIRE(helpers::popContainer<decltype(enumArray)>(ser) == enumArray, per_element());
    BOOST_TEST_REQUIRE(helpers::popContainer<decltype(intEnumArray)>(ser) == intEnumArray, per_element());
}

BOOST_AUTO_TEST_SUITE_END()
