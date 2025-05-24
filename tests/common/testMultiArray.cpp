// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "helpers/MultiArray.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(MultiArrayTests)

static_assert(std::is_same_v<int[1], helpers::detail::AddExtents_t<int, 1>>);
static_assert(std::is_same_v<int[2], helpers::detail::AddExtents_t<int, 2>>);
static_assert(std::is_same_v<int[3][5], helpers::detail::AddExtents_t<int, 3, 5>>);
static_assert(std::is_same_v<int[3][5][7], helpers::detail::AddExtents_t<int, 3, 5, 7>>);

const helpers::MultiArray<int, 3, 2> sma32 = {{{0, 1}, {10, 11}, {20, 21}}};
const helpers::MultiArray<int, 4, 3, 2> sma432 = {{{{0, 1}, {10, 11}, {20, 21}},
                                                   {{100, 101}, {110, 111}, {120, 121}},
                                                   {{200, 201}, {210, 211}, {220, 221}},
                                                   {{300, 301}, {310, 311}, {320, 321}}}};
const helpers::MultiArray<int, 3, 4, 3, 2> sma3432 = {{{{{0, 1}, {10, 11}, {20, 21}},
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

BOOST_AUTO_TEST_CASE(Test2DArray)
{
    BOOST_TEST_REQUIRE(sma32.shape() == (std::array<size_t, 2>{3, 2}));

    BOOST_TEST_REQUIRE(sma32.size() == 3u);
    for(int i = 0; i < 3; i++)
    {
        BOOST_TEST_REQUIRE(sma32[i].size() == 2u);
        for(int j = 0; j < 2; j++)
        {
            BOOST_TEST(sma32[i][j] == i * 10 + j);
            BOOST_TEST(sma32(i, j) == i * 10 + j);
        }
    }

    helpers::MultiArray<int, 3, 2> tmp;
    static_assert(std::is_same_v<decltype(sma32[0][0]), const int&>);
    static_assert(std::is_same_v<decltype(sma32(0, 0)), const int&>);
    static_assert(std::is_same_v<decltype(tmp[0][0]), int&>);
    static_assert(std::is_same_v<decltype(tmp(0, 0)), int&>);

    tmp[0][0] = 0;
    tmp[0][1] = 1;
    tmp[1][0] = 2;
    tmp[1][1] = 3;
    tmp[2][0] = 4;
    tmp[2][1] = 5;
    for(size_t i = 0; i < tmp.numElements(); ++i)
        BOOST_TEST(tmp.data()[i] == static_cast<int>(i));
    int i = 0;
    for(int val : tmp)
    {
        BOOST_TEST(val == i);
        ++i;
    }
}

BOOST_AUTO_TEST_CASE(Test3DArray)
{
    BOOST_TEST_REQUIRE(sma432.shape() == (std::array<size_t, 3>{4, 3, 2}));
    BOOST_TEST_REQUIRE(sma432.size() == 4u);
    for(int i = 0; i < 4; i++)
    {
        BOOST_TEST_REQUIRE(sma432[i].size() == 3u);
        for(int j = 0; j < 3; j++)
        {
            BOOST_TEST_REQUIRE(sma432[i][j].size() == 2u);
            for(int k = 0; k < 2; k++)
            {
                BOOST_TEST(sma432[i][j][k] == i * 100 + j * 10 + k);
                BOOST_TEST(sma432(i, j, k) == i * 100 + j * 10 + k);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(Test4DArray)
{
    BOOST_TEST_REQUIRE(sma3432.shape() == (std::array<size_t, 4>{3, 4, 3, 2}));
    BOOST_TEST_REQUIRE(sma3432.size() == 3u);
    for(int i = 0; i < 3; i++)
    {
        BOOST_TEST_REQUIRE(sma3432[i].size() == 4u);
        for(int j = 0; j < 4; j++)
        {
            BOOST_TEST_REQUIRE(sma3432[i][j].size() == 3u);
            for(int k = 0; k < 3; k++)
            {
                BOOST_TEST_REQUIRE(sma3432[i][j][k].size() == 2u);
                for(int l = 0; l < 2; l++)
                {
                    BOOST_TEST(sma3432[i][j][k][l] == i * 1000 + j * 100 + k * 10 + l);
                    BOOST_TEST(sma3432(i, j, k, l) == i * 1000 + j * 100 + k * 10 + l);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
