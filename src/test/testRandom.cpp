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
#include <boost/random/seed_seq.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(RNG_Tests)

BOOST_AUTO_TEST_CASE(RandomTest)
{
    initGameRNG();
    std::vector<unsigned> resultCt6(6);
    std::vector<unsigned> resultCt13(13);
    std::vector<unsigned> resultCt28(28);
    const unsigned numSamples = 3000;
    for(unsigned i = 0; i < numSamples; i++)
    {
        ++resultCt6.at(RANDOM.Rand(__FILE__, __LINE__, 0, resultCt6.size()));
        ++resultCt13.at(RANDOM.Rand(__FILE__, __LINE__, 0, resultCt13.size()));
        ++resultCt28.at(RANDOM.Rand(__FILE__, __LINE__, 0, resultCt28.size()));
    }
    // Result must be at least 70% of the average
    for(unsigned i = 0; i < resultCt6.size(); i++)
        BOOST_REQUIRE_GT(resultCt6[i], numSamples / resultCt6.size() * 70u / 100u);
    for(unsigned i = 0; i < resultCt13.size(); i++)
        BOOST_REQUIRE_GT(resultCt13[i], numSamples / resultCt13.size() * 70u / 100u);
    for(unsigned i = 0; i < resultCt28.size(); i++)
        BOOST_REQUIRE_GT(resultCt28[i], numSamples / resultCt28.size() * 70u / 100u);
}

BOOST_AUTO_TEST_CASE(XorShiftRange)
{
    XorShift rng;
    const unsigned numSamples = 3000;
    const XorShift::result_type min = rng.min();
    const XorShift::result_type max = rng.max();
    for(unsigned i = 0; i < numSamples; i++){
        XorShift::result_type val = rng();
        BOOST_REQUIRE_GE(val, min);
        BOOST_REQUIRE_LE(val, max);
    }
    std::vector< std::vector<unsigned> > results;
    results.push_back(std::vector<unsigned>(10));
    results.push_back(std::vector<unsigned>(11));
    results.push_back(std::vector<unsigned>(13));
    results.push_back(std::vector<unsigned>(32));
    results.push_back(std::vector<unsigned>(33));
    for(size_t j = 0; j < results.size(); j++){
        std::vector<unsigned>& result = results[j];
        const unsigned maxVal = static_cast<unsigned>(result.size() - 1);
        for(unsigned i = 0; i < numSamples; i++){
            unsigned val = rng(maxVal);
            BOOST_REQUIRE_LE(val, maxVal);
            ++result.at(val);
        }
        // Now every value has to be set (every value was rolled at least once)
        for(unsigned i = 0; i < result.size(); i++)
            BOOST_REQUIRE_GT(result[i], 0u);
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
    for(unsigned i = 0; i < numSamples; i++){
        XorShift::result_type val = rng();
        if(val != firstVal){
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
