// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "helpers/EnumArray.h"
#include "random/DefaultLCG.h"
#include "random/Random.h"
#include "random/XorShift.h"
#include "s25util/Serializer.h"
#include <boost/test/unit_test.hpp>
#include <limits>
#include <random>
#include <utility>
#include <vector>

namespace {
struct SeedFixture
{
    // For every seed the rng must return good random values
    // so try a regular one and some corner cases
    std::vector<unsigned> seeds = {0, 0x1337, std::numeric_limits<unsigned>::max(),
                                   std::numeric_limits<unsigned short>::max()};
};
using TestedRNGS = std::tuple<DefaultLCG, XorShift>;

} // namespace

BOOST_FIXTURE_TEST_SUITE(RNG_Tests, SeedFixture)

BOOST_AUTO_TEST_CASE(RandomTest)
{
    const auto GetObjId = []() { return 0u; }; // Fake function for RANDOM_RAND
    for(unsigned seed : seeds)
    {
        RANDOM.Init(seed);
        std::vector<std::vector<unsigned>> results{{1}, {10}, {11}, {13}, {32}, {33}};
        const unsigned numSamples = 3000;
        for(unsigned i = 0; i < numSamples; i++)
        {
            for(std::vector<unsigned>& result : results)
            {
                // Using .at makes sure we don't exceed the maximum value
                ++result.at(RANDOM_RAND(result.size()));
            }
        }
        // We want a uniform distribution. So all values should occur about the same number of times
        // this is: average = numSamples / maxVal
        // Due to normal fluctuations on true random numbers we just check that they occurred at least
        // average * percentage times
        const unsigned minPercentage = 70u;
        for(std::vector<unsigned>& result : results)
        {
            const unsigned average = numSamples / result.size();
            const unsigned minCt = average * minPercentage / 100u;
            for(unsigned int i : result)
                BOOST_TEST_REQUIRE(i > minCt);
        }
    }
}

BOOST_AUTO_TEST_CASE(RandomSameSeq)
{
    const auto GetObjId = []() { return 0u; }; // Fake function for RANDOM_RAND
    // The rng must return the same sequence of values for a given seed
    RANDOM.Init(0x1337);
    std::vector<int> results = {623, 453, 927, 305, 478, 933, 230, 491, 968, 623, 418};
    for(int result : results)
    {
        // std::cout << RANDOM_RAND(1024) << std::endl;
        BOOST_TEST_REQUIRE(RANDOM_RAND(1024) == result);
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(ValueRangeValid, T_RNG, TestedRNGS)
{
    for(unsigned seed : seeds)
    {
        T_RNG rng(seed);
        const unsigned numSamplesMinMax = 3000;
        constexpr auto min = rng.min();
        constexpr auto max = rng.max();
        for(unsigned i = 0; i < numSamplesMinMax; i++)
        {
            const auto val = rng();
            BOOST_TEST_REQUIRE(val >= min);
            BOOST_TEST_REQUIRE(val <= max);
        }

        std::vector<std::vector<unsigned>> results = {{2}, {10}, {11}, {13}, {32}, {33}};
        const unsigned numSamples = 3000;
        for(unsigned i = 0; i < numSamples; i++)
        {
            for(std::vector<unsigned>& result : results)
            {
                const auto maxVal = static_cast<unsigned>(result.size());
                unsigned val = rng() % maxVal;
                ++result[val];
            }
        }
        // We want a uniform distribution. So all values should occur about the same number of times
        // this is: average = numSamples / maxVal
        // Due to normal fluctuations on true random numbers we just check that they occurred at least
        // average * percentage times
        const unsigned minPercentage = 70u;
        for(std::vector<unsigned>& result : results)
        {
            const unsigned average = numSamples / result.size();
            const unsigned minCt = average * minPercentage / 100u;
            for(unsigned int i : result)
                BOOST_TEST_REQUIRE(i > minCt);
        }
    }
}

BOOST_AUTO_TEST_CASE(EmptyRange)
{
    const auto GetObjId = []() { return 0u; }; // Fake function for RANDOM_RAND
    for(unsigned seed : seeds)
    {
        RANDOM.Init(seed);
        for(int i = 0; i < 100; i++)
        {
            // Create a random number in [0, 0] is always 0
            BOOST_TEST_REQUIRE(RANDOM_RAND(0) == 0);
        }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(CtorFromSeedSeq, T_RNG, TestedRNGS)
{
    std::seed_seq seedSeq(seeds.begin(), seeds.end());
    T_RNG rng(seedSeq);
    // Now check that we get different values during multiple calls
    // to the rng. It is possible that same values are returned as it is random
    // but it is unlikely for many values to be the same
    const unsigned numSamples = 10;
    const auto firstVal = rng();
    bool differentValueFound = false;
    for(unsigned i = 0; i < numSamples; i++)
    {
        const auto val = rng();
        if(val != firstVal)
        {
            differentValueFound = true;
            break;
        }
    }
    BOOST_TEST_REQUIRE(differentValueFound);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(Copy, T_RNG, TestedRNGS)
{
    for(unsigned seed : seeds)
    {
        T_RNG rng(seed), rng2(seed);
        BOOST_TEST_REQUIRE(rng == rng2);
        const auto val = rng();
        BOOST_TEST_REQUIRE(rng != rng2);
        // Both must return the same value
        BOOST_TEST_REQUIRE(val == rng2());

        // Execute it a few times
        for(unsigned i = 0; i < 10; i++)
            rng();
        BOOST_TEST_REQUIRE(rng != rng2);
        rng2 = rng;
        BOOST_TEST_REQUIRE(rng == rng2);
        // Both must return the same value
        BOOST_TEST_REQUIRE(rng() == rng2());
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(StreamOperations, T_RNG, TestedRNGS)
{
    for(unsigned seed : seeds)
    {
        T_RNG rng(seed), rng2(seed);
        // Execute it a few times
        for(unsigned i = 0; i < 10; i++)
            rng();
        std::stringstream ss;
        // Save to stream
        ss << rng;
        // Load it
        ss >> rng2;
        BOOST_TEST_REQUIRE(rng == rng2);
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(Serialization, T_RNG, TestedRNGS)
{
    for(unsigned seed : seeds)
    {
        T_RNG rng(seed), rng2(seed);
        // Execute it a few times
        for(unsigned i = 0; i < 10; i++)
            rng();
        Serializer ser;
        rng.serialize(ser);
        rng2.deserialize(ser);
        BOOST_TEST_REQUIRE(rng == rng2);
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(Discard, T_RNG, TestedRNGS)
{
    for(unsigned seed : seeds)
    {
        T_RNG rng(seed), rng2(seed);
        const unsigned skipCt = 100;
        // Execute it a few times
        for(unsigned i = 0; i < skipCt; i++)
            rng();
        rng2.discard(skipCt);
        BOOST_TEST_REQUIRE(rng == rng2);
    }
}

namespace {
enum class TestEnum
{
    e1,
    e2,
    e3
};
constexpr auto maxEnumValue(TestEnum)
{
    return TestEnum::e3;
}
} // namespace

BOOST_AUTO_TEST_CASE(RandomEnum)
{
    const auto GetObjId = []() { return 0u; }; // Fake function for RANDOM_ENUM
    helpers::EnumArray<bool, TestEnum> triggered{};
    for(unsigned seed : seeds)
    {
        RANDOM.Init(seed);
        triggered = {};
        for(int i = 0; i < 30; i++)
            triggered[RANDOM_ENUM(TestEnum)] = true;
        BOOST_TEST(triggered[TestEnum::e1]);
        BOOST_TEST(triggered[TestEnum::e2]);
        BOOST_TEST(triggered[TestEnum::e3]);
    }
}

BOOST_AUTO_TEST_CASE(RandomElement)
{
    const auto GetObjId = []() { return 0u; }; // Fake function for RANDOM_ENUM
    std::array<bool, 3> triggered{};
    const std::array<int, 3> indices{0, 1, 2};
    for(unsigned seed : seeds)
    {
        RANDOM.Init(seed);
        triggered = {};
        for(int i = 0; i < 30; i++)
            triggered[RANDOM_ELEMENT(indices)] = true;
        BOOST_TEST(triggered[0]);
        BOOST_TEST(triggered[1]);
        BOOST_TEST(triggered[2]);
    }
}

BOOST_AUTO_TEST_SUITE_END()
