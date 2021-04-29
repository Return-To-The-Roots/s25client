// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "enum_cast.hpp"
#include "helpers/EnumRange.h"
#include "gameTypes/Direction.h"
#include "gameTypes/DirectionToImgDir.h"
#include "gameTypes/GameTypesOutput.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(DirectionCmp)
{
    Direction east(Direction::East); //-V525
    Direction east2(Direction::East);
    Direction west(Direction::West);
    // All variations: Dir-Dir, Dir-Type, Type-Dir
    BOOST_TEST_REQUIRE(east == east2);
    BOOST_TEST_REQUIRE(east == Direction::East);
    BOOST_TEST_REQUIRE(Direction::East == east2);
    BOOST_TEST_REQUIRE(east != west);
    BOOST_TEST_REQUIRE(east != Direction::West);
    BOOST_TEST_REQUIRE(Direction::West != east2);
}

BOOST_AUTO_TEST_CASE(DirectionIncDec)
{
    // For every direction
    for(const auto startDir : helpers::enumRange<Direction>())
    {
        // Fit back to range
        const auto iStartDir = rttr::enum_cast(startDir);
        BOOST_TEST_REQUIRE(rttr::enum_cast(convertToDirection(iStartDir + helpers::NumEnumValues_v<Direction>))
                           == iStartDir);
        for(unsigned diff = 0; diff < 20; diff++)
        {
            // Add
            const Direction expectedDir = convertToDirection(iStartDir + diff);
            BOOST_TEST_REQUIRE(startDir + diff == expectedDir);
            Direction resultDir = startDir;
            BOOST_TEST_REQUIRE((resultDir += diff) == expectedDir);
            BOOST_TEST_REQUIRE(resultDir == expectedDir);

            // Subtract
            const Direction expectedDir2 =
              convertToDirection(iStartDir + helpers::NumEnumValues_v<Direction> * 10 - diff);
            BOOST_TEST_REQUIRE(startDir - diff == expectedDir2);
            resultDir = startDir;
            BOOST_TEST_REQUIRE((resultDir -= diff) == expectedDir2);
            BOOST_TEST_REQUIRE(resultDir == expectedDir2);
        }
    }
}

BOOST_AUTO_TEST_CASE(DirectionToImgDir)
{
    for(Direction curDir : helpers::EnumRange<Direction>{})
    {
        // S2 Img dir is offset by 3
        BOOST_TEST(static_cast<unsigned>(toImgDir(curDir)) == rttr::enum_cast(curDir + 3u));
    }
}
