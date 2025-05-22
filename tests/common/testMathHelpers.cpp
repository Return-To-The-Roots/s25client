// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "helpers/SmoothedValue.hpp"
#include "helpers/mathFuncs.h"
#include "helpers/roundToNextPow2.h"
#include <rttr/test/random.hpp>
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <utility>

BOOST_AUTO_TEST_SUITE(MathHelperTests)

BOOST_AUTO_TEST_CASE(NextPow2)
{
    BOOST_TEST(helpers::roundToNextPowerOfTwo(0) == 1u);
    BOOST_TEST(helpers::roundToNextPowerOfTwo(1) == 1u);
    BOOST_TEST(helpers::roundToNextPowerOfTwo(2) == 2u);
    BOOST_TEST(helpers::roundToNextPowerOfTwo(3) == 4u);
    BOOST_TEST(helpers::roundToNextPowerOfTwo(4) == 4u);
    BOOST_TEST(helpers::roundToNextPowerOfTwo(5) == 8u);
    // Start with 2^3=8 to 2^30
    for(unsigned shift = 3; shift < 30; shift++)
    {
        unsigned val = 1u << shift; // 2^shift
        // Pow2 stays unchanged
        BOOST_TEST(helpers::roundToNextPowerOfTwo(val) == val);
        // -1 -> val
        BOOST_TEST(helpers::roundToNextPowerOfTwo(val - 1u) == val);
        // +1 -> val * 2
        BOOST_TEST(helpers::roundToNextPowerOfTwo(val + 1u) == val * 2);
    }
    BOOST_TEST(helpers::roundToNextPowerOfTwo(1u << 31) == 1u << 31);
    // TODO: Max unsigned is 2^32 - 1. What to do with values greater than 2^31?
}

BOOST_AUTO_TEST_CASE(clamp)
{
    // Simple
    BOOST_TEST(helpers::clamp(5, 1, 10) == 5);
    BOOST_TEST(helpers::clamp(-5, 1, 10) == 1);
    BOOST_TEST(helpers::clamp(0, 1, 10) == 1);
    BOOST_TEST(helpers::clamp(1, 1, 10) == 1);
    BOOST_TEST(helpers::clamp(10, 1, 10) == 10);
    BOOST_TEST(helpers::clamp(11, 1, 10) == 10);
    // Signed value, unsigned min/max
    BOOST_TEST(helpers::clamp(5, 1u, 10u) == 5u);
    BOOST_TEST(helpers::clamp(-5, 1u, 10u) == 1u);
    BOOST_TEST(helpers::clamp(std::numeric_limits<int>::min(), 1u, 10u) == 1u);
    BOOST_TEST(helpers::clamp(std::numeric_limits<int>::max(), 1u, 10u) == 10u);
    // unsigned value, signed min/max
    BOOST_TEST(helpers::clamp(5u, -5, 6) == 5);
    BOOST_TEST(helpers::clamp(10u, -5, 6) == 6);
    BOOST_TEST(helpers::clamp(std::numeric_limits<unsigned>::min(), -5, 6) == 0);
    BOOST_TEST(helpers::clamp(std::numeric_limits<unsigned>::max(), -5, 6) == 6);
    BOOST_TEST(helpers::clamp(std::numeric_limits<unsigned>::min(), -15, -2) == -2);
    BOOST_TEST(helpers::clamp(std::numeric_limits<unsigned>::max(), -15, -2) == -2);
    BOOST_TEST(helpers::clamp(std::numeric_limits<unsigned>::min(), std::numeric_limits<int>::min(), 10) == 0);
    BOOST_TEST(helpers::clamp(std::numeric_limits<unsigned>::max(), std::numeric_limits<int>::min(), 10) == 10);
    BOOST_TEST(helpers::clamp(std::numeric_limits<unsigned>::min(), std::numeric_limits<int>::min(),
                              std::numeric_limits<int>::max())
               == 0);
    BOOST_TEST(helpers::clamp(std::numeric_limits<unsigned>::max(), std::numeric_limits<int>::min(),
                              std::numeric_limits<int>::max())
               == std::numeric_limits<int>::max());
    // short value, long min/max
    BOOST_TEST(helpers::clamp(static_cast<short>(-3), -5, 6) == -3);
    BOOST_TEST(helpers::clamp(static_cast<short>(5), -5, 6) == 5);
    BOOST_TEST(helpers::clamp(std::numeric_limits<short>::min(), -5, 6) == -5);
    BOOST_TEST(helpers::clamp(std::numeric_limits<short>::max(), -5, 6) == 6);
    // long value, short min/max
    BOOST_TEST(helpers::clamp(-1, static_cast<short>(-3), static_cast<short>(6)) == -1);
    BOOST_TEST(helpers::clamp(4, static_cast<short>(-3), static_cast<short>(6)) == 4);
    BOOST_TEST(helpers::clamp(std::numeric_limits<int>::min(), static_cast<short>(-3), static_cast<short>(6)) == -3);
    BOOST_TEST(helpers::clamp(std::numeric_limits<int>::max(), static_cast<short>(-3), static_cast<short>(6)) == 6);
}

BOOST_AUTO_TEST_CASE(SmoothedValue)
{
    helpers::SmoothedValue<int> val(5);
    BOOST_TEST_REQUIRE(val.size() == 0u);
    BOOST_TEST_REQUIRE(val.get() == 0);
    val.add(4);
    BOOST_TEST_REQUIRE(val.size() == 1u);
    BOOST_TEST_REQUIRE(val.get() == 4);
    val.add(2);
    BOOST_TEST_REQUIRE(val.size() == 2u);
    BOOST_TEST_REQUIRE(val.get() == (4 + 2) / 2);
    val.add(11);
    BOOST_TEST_REQUIRE(val.size() == 3u);
    BOOST_TEST_REQUIRE(val.get() == (4 + 2 + 11) / 3);
    val.add(102);
    BOOST_TEST_REQUIRE(val.size() == 4u);
    BOOST_TEST_REQUIRE(val.get() == (4 + 2 + 11 + 102) / 4);
    val.add(1);
    BOOST_TEST_REQUIRE(val.size() == 5u);
    BOOST_TEST_REQUIRE(val.get() == (4 + 2 + 11 + 102 + 1) / 5);
    val.add(7);
    BOOST_TEST_REQUIRE(val.size() == 5u);
    BOOST_TEST_REQUIRE(val.get() == (2 + 11 + 102 + 1 + 7) / 5);
}

BOOST_AUTO_TEST_CASE(DivCeilResults)
{
    BOOST_TEST(helpers::divCeil(0, 3) == 0u);
    BOOST_TEST(helpers::divCeil(1, 3) == 1u);
    BOOST_TEST(helpers::divCeil(2, 3) == 1u);
    BOOST_TEST(helpers::divCeil(3, 3) == 1u);
    BOOST_TEST(helpers::divCeil(4, 3) == 2u);
    BOOST_TEST(helpers::divCeil(5, 3) == 2u);
    BOOST_TEST(helpers::divCeil(6, 3) == 2u);

    BOOST_TEST(helpers::divCeil(1, 8) == 1u);
    BOOST_TEST(helpers::divCeil(2, 8) == 1u);
    BOOST_TEST(helpers::divCeil(3, 8) == 1u);
    BOOST_TEST(helpers::divCeil(4, 8) == 1u);
    BOOST_TEST(helpers::divCeil(5, 8) == 1u);
    BOOST_TEST(helpers::divCeil(6, 8) == 1u);
    BOOST_TEST(helpers::divCeil(7, 8) == 1u);
    BOOST_TEST(helpers::divCeil(8, 8) == 1u);
    BOOST_TEST(helpers::divCeil(9, 8) == 2u);
}

using TimeTypes = std::tuple<std::chrono::duration<int32_t, std::milli>, std::chrono::duration<uint32_t, std::milli>>;
template<typename T>
struct Rep
{
    using type = T;
};
template<typename T, typename U>
struct Rep<std::chrono::duration<T, U>>
{
    using type = T;
};

BOOST_AUTO_TEST_CASE_TEMPLATE(Interpolate, T, TimeTypes)
{
    using RepT = typename Rep<T>::type;
    using rttr::test::randomValue;
    {
        const auto duration = randomValue<RepT>(1, 0xFFFF / 2);
        const auto maxStepSize = std::max<unsigned>(1, duration / 100);
        for(int i = 0; i <= static_cast<int>(duration); i += randomValue<int>(1, maxStepSize))
        {
            const int maxVal(duration * 2);
            const int minVal2(duration);
            BOOST_TEST(helpers::interpolate(0, maxVal, T(i), T(duration)) == 2 * i);
            BOOST_TEST(helpers::interpolate(maxVal, 0, T(i), T(duration)) == maxVal - 2 * i);
            BOOST_TEST(helpers::interpolate(minVal2, maxVal, T(i), T(duration)) == minVal2 + i);
            BOOST_TEST(helpers::interpolate(maxVal, minVal2, T(i), T(duration)) == maxVal - i);
        }
        for(unsigned i = 0; i <= static_cast<unsigned>(duration); i += randomValue<int>(1, maxStepSize))
        {
            const unsigned maxVal(duration * 2);
            const unsigned minVal2(duration);
            BOOST_TEST(helpers::interpolate(0u, maxVal, T(i), T(duration)) == 2u * i);
            BOOST_TEST(helpers::interpolate(maxVal, 0u, T(i), T(duration)) == maxVal - 2u * i);
            BOOST_TEST(helpers::interpolate(minVal2, maxVal, T(i), T(duration)) == minVal2 + i);
            BOOST_TEST(helpers::interpolate(maxVal, minVal2, T(i), T(duration)) == maxVal - i);
        }
    }
    {
        const auto startVal = randomValue<int>(-1000, 1000);
        const auto endVal = randomValue<int>(startVal + 10, startVal + 1000);
        if(std::is_signed_v<RepT>)
        {
            // Elapsed time less than zero
            BOOST_TEST(helpers::interpolate(startVal, endVal, T(randomValue<int>(-10000, -1)), T(randomValue<int>(1)))
                       == startVal);
        }
        // Elapsed time greater than duration
        const auto duration = randomValue<RepT>(1, 5000000);
        BOOST_TEST(helpers::interpolate(startVal, endVal, T(randomValue<RepT>(duration)), T(duration)) == endVal);
    }
    {
        const auto startVal = randomValue<unsigned>(0u, 500000u);
        const auto endVal = randomValue<unsigned>(startVal + 10u);
        if(std::is_signed_v<RepT>)
        {
            // Elapsed time less than zero
            BOOST_TEST(helpers::interpolate(startVal, endVal, T(randomValue<int>(-10000, -1)), T(randomValue<int>(1)))
                       == startVal);
        }
        // Elapsed time greater than duration
        const auto duration = randomValue<RepT>(1, 5000000);
        BOOST_TEST(helpers::interpolate(startVal, endVal, T(randomValue<RepT>(duration)), T(duration)) == endVal);
    }
}

BOOST_AUTO_TEST_CASE(Lerp)
{
    const auto startVal = 5.0f;
    const auto endVal = 10.0f;
    BOOST_TEST(helpers::lerp(startVal, endVal, .0f) == startVal);
    BOOST_TEST(helpers::lerp(startVal, endVal, 1.0f) == endVal);
    BOOST_TEST(helpers::lerp(startVal, endVal, .5f) == 7.5f);
    BOOST_TEST(helpers::lerp(startVal, endVal, 2.0f) == 15.0f);
    BOOST_TEST(helpers::lerp(startVal, endVal, -.5f) == 2.5f);
}

BOOST_AUTO_TEST_CASE(InverseLerp)
{
    const auto startVal = 5.0f;
    const auto endVal = 10.0f;
    BOOST_TEST(helpers::inverseLerp(startVal, endVal, startVal) == .0f);
    BOOST_TEST(helpers::inverseLerp(startVal, endVal, endVal) == 1.0f);
    BOOST_TEST(helpers::inverseLerp(startVal, endVal, 7.5f) == .5f);
    BOOST_TEST(helpers::inverseLerp(startVal, endVal, 15.0f) == 2.0f);
    BOOST_TEST(helpers::inverseLerp(startVal, endVal, 2.5f) == -.5f);
}

BOOST_AUTO_TEST_SUITE_END()
