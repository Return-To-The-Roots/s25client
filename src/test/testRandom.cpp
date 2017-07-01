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

#include "defines.h" // IWYU pragma: keep
#include "Random.h"
#include "random/XorShift.h"
#include "test/testHelpers.h"
#include "libutil/src/Serializer.h"
#include <boost/test/unit_test.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/random/seed_seq.hpp>
#include <boost/foreach.hpp>
#include <vector>
#include <limits>
//#include <iostream>

using namespace boost::assign;

namespace{
    struct SeedFixture{
        SeedFixture()
        {
            // For every seed the rng must return good random values
            // so try a regular one and some corner cases
            seeds += 0x1337, 0, std::numeric_limits<unsigned>::max(), std::numeric_limits<unsigned short>::max();
        }
        std::vector<unsigned> seeds;
    };
}

BOOST_AUTO_TEST_SUITE(RNG_Tests)

BOOST_FIXTURE_TEST_CASE(RandomTest, SeedFixture)
{
    BOOST_FOREACH(unsigned seed, seeds)
    {
        RANDOM.Init(seed);
        std::vector< std::vector<unsigned> > results;
        results += std::vector<unsigned>(1), std::vector<unsigned>(10), std::vector<unsigned>(11),
            std::vector<unsigned>(13), std::vector<unsigned>(32), std::vector<unsigned>(33);
        const unsigned numSamples = 3000;
        for(unsigned i = 0; i < numSamples; i++)
        {
            BOOST_FOREACH(std::vector<unsigned>& result, results)
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
        BOOST_FOREACH(std::vector<unsigned>& result, results)
        {
            const unsigned average = numSamples / result.size();
            const unsigned minCt = average * minPercentage / 100u;
            for(unsigned i = 0; i < result.size(); i++)
                BOOST_REQUIRE_GT(result[i], minCt);
        }
    }
}

BOOST_AUTO_TEST_CASE(RandomSameSeq)
{
    // The rng must return the same sequence of values for a given seed
    RANDOM.Init(0x1337);
    std::vector<int> results;
    results += 713, 860, 519, 141, 414, 616, 313, 458, 421, 302;
    BOOST_FOREACH(int result, results)
    {
        //std::cout << RANDOM_RAND(0, 1024) << std::endl;
        BOOST_REQUIRE_EQUAL(RANDOM_RAND(0, 1024), result);
    }
}

BOOST_FIXTURE_TEST_CASE(RandomEmptySeq, SeedFixture)
{
    BOOST_FOREACH(unsigned seed, seeds)
    {
        RANDOM.Init(seed);
        for(int i = 0; i < 100; i++)
        {
            // Create a random number in [0, 0) is always 0 (by definition)
            BOOST_REQUIRE_EQUAL(RANDOM_RAND(0, 0), 0);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(XorShiftRange, SeedFixture)
{
    BOOST_FOREACH(unsigned seed, seeds)
    {
        XorShift rng(seed);
        const unsigned numSamplesMinMax = 3000;
        const XorShift::result_type min = rng.min();
        const XorShift::result_type max = rng.max();
        for(unsigned i = 0; i < numSamplesMinMax; i++)
        {
            XorShift::result_type val = rng();
            BOOST_REQUIRE_GE(val, min);
            BOOST_REQUIRE_LE(val, max);
        }

        std::vector< std::vector<unsigned> > results;
        results += std::vector<unsigned>(1), std::vector<unsigned>(10), std::vector<unsigned>(11),
            std::vector<unsigned>(13), std::vector<unsigned>(32), std::vector<unsigned>(33);
        const unsigned numSamples = 2000;
        for(unsigned i = 0; i < numSamples; i++)
        {
            BOOST_FOREACH(std::vector<unsigned>& result, results)
            {
                const unsigned maxVal = static_cast<unsigned>(result.size() - 1);
                unsigned val = rng(maxVal);
                BOOST_REQUIRE_LE(val, maxVal);
                // Using .at makes sure we don't exceed the maximum value
                ++result.at(val);
            }
        }
        // We want a uniform distribution. So all values should occur about the same number of times
        // this is: average = numSamples / maxVal
        // Due to normal fluctuations on true random numbers we just check that they occurred at least
        // average * percentage times
        const unsigned minPercentage = 70u;
        BOOST_FOREACH(std::vector<unsigned>& result, results)
        {
            const unsigned average = numSamples / result.size();
            const unsigned minCt = average * minPercentage / 100u;
            for(unsigned i = 0; i < result.size(); i++)
                BOOST_REQUIRE_GT(result[i], minCt);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(XorShiftEmptyRange, SeedFixture)
{
    BOOST_FOREACH(unsigned seed, seeds)
    {
        XorShift rng(seed);
        for(int i = 0; i < 100; i++)
        {
            // Create a random number in [0, 0] is always 0
            BOOST_REQUIRE_EQUAL(rng(0), 0u);
        }
    }
}

BOOST_AUTO_TEST_CASE(XorShiftFromSeedSeq)
{
    std::vector<unsigned> seeds;
    // Just add a few random values
    seeds.push_back(0);
    seeds.push_back(13455);
    seeds.push_back(0x1337);
    boost::random::seed_seq seedSeq(seeds.begin(), seeds.end());
    XorShift rng(seedSeq);
    // Now check that we get different values during multiple calls
    // to the rng. It is possible that same values are returned as it is random
    // but it is unlikely for many values to be the same
    const unsigned numSamples = 10;
    const XorShift::result_type firstVal = rng();
    bool differentValueFound = false;
    for(unsigned i = 0; i < numSamples; i++)
    {
        XorShift::result_type val = rng();
        if(val != firstVal)
        {
            differentValueFound = true;
            break;
        }
    }
    BOOST_REQUIRE(differentValueFound);
}

BOOST_AUTO_TEST_CASE(XorShiftCopy)
{
    XorShift rng, rng2;
    BOOST_REQUIRE_EQUAL(rng, rng2);
    XorShift::result_type val = rng();
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

BOOST_AUTO_TEST_CASE(XorShiftStream)
{
    XorShift rng, rng2;
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

BOOST_AUTO_TEST_CASE(XorShiftSerialize)
{
    XorShift rng, rng2;
    // Execute it a few times
    for(unsigned i = 0; i < 10; i++)
        rng();
    Serializer ser;
    rng.Serialize(ser);
    rng2.Deserialize(ser);
    BOOST_REQUIRE_EQUAL(rng, rng2);
}

BOOST_AUTO_TEST_CASE(XorShiftDiscard)
{
    XorShift rng, rng2;
    const unsigned skipCt = 100;
    // Execute it a few times
    for(unsigned i = 0; i < skipCt; i++)
        rng();
    rng2.discard(skipCt);
    BOOST_REQUIRE_EQUAL(rng, rng2);
}

BOOST_AUTO_TEST_SUITE_END()
