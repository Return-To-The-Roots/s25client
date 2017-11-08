// Copyright (c) 2016 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "helpers/SimpleMultiArray.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(MultiArrayTests)

const helpers::SimpleMultiArray<int, 3, 2> sma32 = {{{0, 1}, {10, 11}, {20, 21}}};
const helpers::SimpleMultiArray<int, 4, 3, 2> sma432 = {{{{0, 1}, {10, 11}, {20, 21}},
                                                         {{100, 101}, {110, 111}, {120, 121}},
                                                         {{200, 201}, {210, 211}, {220, 221}},
                                                         {{300, 301}, {310, 311}, {320, 321}}}};
const helpers::SimpleMultiArray<int, 3, 4, 3, 2> sma3432 = {{{{{0, 1}, {10, 11}, {20, 21}},
                                                              {{100, 101}, {110, 111}, {120, 121}},
                                                              {{200, 201}, {210, 211}, {220, 221}},
                                                              {{300, 301}, {310, 311}, {320, 321}}},
                                                             {{{1000, 1001}, {1010, 1011}, {1020, 1021}},
                                                              {{1100, 1101}, {1110, 1111}, {1120, 1121}},
                                                              {{1200, 1201}, {1210, 1211}, {1220, 1221}},
                                                              {{1300, 1301}, {1310, 1311}, {1320, 1321}}},
                                                             {{{2000, 2001}, {2010, 2011}, {2020, 2021}},
                                                              {{2100, 2101}, {2110, 2111}, {2120, 2121}},
                                                              {{2200, 2201}, {2210, 2211}, {2220, 2221}},
                                                              {{2300, 2301}, {2310, 2311}, {2320, 2321}}}}};

BOOST_AUTO_TEST_CASE(SimpleMultiArrayTest)
{
    BOOST_REQUIRE_EQUAL(sma32.size(), 3u);
    for(int i = 0; i < 3; i++)
    {
        BOOST_REQUIRE_EQUAL(sma32[i].size(), 2u);
        for(int j = 0; j < 2; j++)
            BOOST_REQUIRE_EQUAL(sma32[i][j], i * 10 + j);
    }
    BOOST_REQUIRE_EQUAL(sma432.size(), 4u);
    for(int i = 0; i < 4; i++)
    {
        BOOST_REQUIRE_EQUAL(sma432[i].size(), 3u);
        for(int j = 0; j < 3; j++)
        {
            BOOST_REQUIRE_EQUAL(sma432[i][j].size(), 2u);
            for(int k = 0; k < 2; k++)
                BOOST_REQUIRE_EQUAL(sma432[i][j][k], i * 100 + j * 10 + k);
        }
    }
    BOOST_REQUIRE_EQUAL(sma3432.size(), 3u);
    for(int i = 0; i < 3; i++)
    {
        BOOST_REQUIRE_EQUAL(sma3432[i].size(), 4u);
        for(int j = 0; j < 4; j++)
        {
            BOOST_REQUIRE_EQUAL(sma3432[i][j].size(), 3u);
            for(int k = 0; k < 3; k++)
            {
                BOOST_REQUIRE_EQUAL(sma3432[i][j][k].size(), 2u);
                for(int l = 0; l < 2; l++)
                    BOOST_REQUIRE_EQUAL(sma3432[i][j][k][l], i * 1000 + j * 100 + k * 10 + l);
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
