// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "gameTypes/Direction.h"
#include "gameTypes/DirectionToImgDir.h"
#include "gameTypes/Direction_Output.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(DirectionCmp)
{
    Direction east(Direction::EAST); //-V525
    Direction east2(Direction::EAST);
    Direction west(Direction::WEST);
    // All variations: Dir-Dir, Dir-Type, Type-Dir
    BOOST_REQUIRE_EQUAL(east, east2);
    BOOST_REQUIRE_EQUAL(east, Direction::EAST);
    BOOST_REQUIRE_EQUAL(Direction::EAST, east2);
    BOOST_REQUIRE_NE(east, west);
    BOOST_REQUIRE_NE(east, Direction::WEST);
    BOOST_REQUIRE_NE(Direction::WEST, east2);
    Direction dir;
}

BOOST_AUTO_TEST_CASE(DirectionIncDec)
{
    // For every direction
    for(unsigned startDir = 0; startDir < Direction::COUNT; startDir++)
    {
        // Fit back to range
        BOOST_REQUIRE_EQUAL(Direction(startDir + Direction::COUNT).toUInt(), startDir);
        // Increment
        Direction testDir(startDir);
        BOOST_REQUIRE_EQUAL(testDir++, Direction(startDir));
        BOOST_REQUIRE_EQUAL(testDir, Direction(startDir + 1));
        BOOST_REQUIRE_EQUAL(++testDir, Direction(startDir + 2));
        BOOST_REQUIRE_EQUAL(testDir, Direction(startDir + 2));
        // Decrement
        BOOST_REQUIRE_EQUAL(testDir--, Direction(startDir + 2));
        BOOST_REQUIRE_EQUAL(testDir, Direction(startDir + 1));
        BOOST_REQUIRE_EQUAL(--testDir, Direction(startDir));
        BOOST_REQUIRE_EQUAL(testDir, Direction(startDir));
        // Add/Subtract. Test using the already tested primitives
        for(unsigned diff = 1; diff < 20; diff++)
        {
            Direction resultDir = testDir + diff;
            Direction expectedDir(testDir);
            for(unsigned i = 0; i < diff; i++)
                ++expectedDir;
            BOOST_REQUIRE_EQUAL(resultDir, expectedDir);
            Direction resultDir2 = testDir;
            BOOST_REQUIRE_EQUAL(resultDir2 += diff, expectedDir);
            BOOST_REQUIRE_EQUAL(resultDir2, expectedDir);
            resultDir = testDir - diff;
            expectedDir = testDir;
            for(unsigned i = 0; i < diff; i++)
                --expectedDir;
            BOOST_REQUIRE_EQUAL(resultDir, expectedDir);
            resultDir2 = testDir;
            BOOST_REQUIRE_EQUAL(resultDir2 -= diff, expectedDir);
            BOOST_REQUIRE_EQUAL(resultDir2, expectedDir);
        }
    }
}

/*
BOOST_AUTO_TEST_CASE(DirectionIteratorWithOffset)
{
    for(unsigned i = 0; i < Direction::COUNT; i++)
    {
        Direction dir(i);
        unsigned ct = 0;
        Direction expectedDir(dir);
        for(auto curDir : dir)
        {
            BOOST_REQUIRE_EQUAL(curDir, expectedDir);
            ++ct;
            ++expectedDir;
        }
        unsigned expectedCt = Direction::COUNT;
        BOOST_REQUIRE_EQUAL(ct, expectedCt);
    }
}*/

BOOST_AUTO_TEST_CASE(DirectionToImgDir)
{
    for(Direction curDir : helpers::EnumRange<Direction>{})
    {
        // S2 Img dir is offset by 3
        BOOST_TEST(static_cast<unsigned>(toImgDir(curDir)) == (static_cast<unsigned>(curDir) + 3) % 6);
    }
}
