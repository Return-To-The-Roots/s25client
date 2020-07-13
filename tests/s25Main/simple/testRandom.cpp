// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "random/DefaultLCG.h"
#include "random/Random.h"
#include "random/XorShift.h"
#include "s25util/Serializer.h"
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <limits>
#include <random>
#include <vector>

namespace {
struct SeedFixture
{
    // For every seed the rng must return good random values
    // so try a regular one and some corner cases
    std::vector<unsigned> seeds = {0, 0x1337, std::numeric_limits<unsigned>::max(), std::numeric_limits<unsigned short>::max()};
};
using TestedRNGS = boost::mpl::list<DefaultLCG, XorShift>;

} // namespace

BOOST_FIXTURE_TEST_SUITE(RNG_Tests, SeedFixture)

BOOST_AUTO_TEST_CASE(RandomTest)
{
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
                ++result.at(RANDOM_RAND(0, result.size()));
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
                BOOST_REQUIRE_GT(i, minCt);
        }
    }
}

BOOST_AUTO_TEST_CASE(RandomSameSeq)
{
    // The rng must return the same sequence of values for a given seed
    RANDOM.Init(0x1337);
    std::vector<int> results = {623, 453, 927, 305, 478, 933, 230, 491, 968, 623, 418};
    for(int result : results)
    {
        // std::cout << RANDOM_RAND(0, 1024) << std::endl;
        BOOST_REQUIRE_EQUAL(RANDOM_RAND(0, 1024), result);
    }
}

BOOST_AUTO_TEST_CASE(RandomEmptySeq)
{
    for(unsigned seed : seeds)
    {
        RANDOM.Init(seed);
        for(int i = 0; i < 100; i++)
        {
            // Create a random number in [0, 0) is always 0 (by definition)
            BOOST_REQUIRE_EQUAL(RANDOM_RAND(0, 0), 0);
        }
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
            BOOST_REQUIRE_GE(val, min);
            BOOST_REQUIRE_LE(val, max);
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
                BOOST_REQUIRE_GT(i, minCt);
        }
    }
}

BOOST_AUTO_TEST_CASE(EmptyRange)
{
    for(unsigned seed : seeds)
    {
        RANDOM.Init(seed);
        for(int i = 0; i < 100; i++)
        {
            // Create a random number in [0, 0] is always 0
            BOOST_REQUIRE_EQUAL(RANDOM_RAND(1337, 0), 0);
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
    BOOST_REQUIRE(differentValueFound);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(Copy, T_RNG, TestedRNGS)
{
    for(unsigned seed : seeds)
    {
        T_RNG rng(seed), rng2(seed);
        BOOST_REQUIRE_EQUAL(rng, rng2);
        const auto val = rng();
        BOOST_REQUIRE_NE(rng, rng2);
        // Both must return the same value
        BOOST_REQUIRE_EQUAL(val, rng2());

        // Execute it a few times
        for(unsigned i = 0; i < 10; i++)
            rng();
        BOOST_REQUIRE_NE(rng, rng2);
        rng2 = rng;
        BOOST_REQUIRE_EQUAL(rng, rng2);
        // Both must return the same value
        BOOST_REQUIRE_EQUAL(rng(), rng2());
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
        BOOST_REQUIRE_EQUAL(rng, rng2);
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
        BOOST_REQUIRE_EQUAL(rng, rng2);
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
        BOOST_REQUIRE_EQUAL(rng, rng2);
    }
}

BOOST_AUTO_TEST_SUITE_END()
