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
#include "test/testHelpers.h"
#include <boost/test/unit_test.hpp>

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

BOOST_AUTO_TEST_SUITE_END()
