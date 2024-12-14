// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CheatCommandTracker.h"
#include "Cheats.h"
#include "driver/KeyEvent.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"

BOOST_AUTO_TEST_SUITE(CheatCommandTrackerTests)

namespace {
template<unsigned T_numPlayers>
struct CheatCommandTrackerFixture : WorldFixture<CreateEmptyWorld, T_numPlayers>
{
    Cheats cheats_{this->world};
    CheatCommandTracker tracker_{cheats_};

    KeyEvent makeKeyEvent(unsigned c) { return {KeyType::Char, c, false, false, false}; }
    KeyEvent makeKeyEvent(KeyType kt) { return {kt, 0, false, false, false}; }

    void trackString(const std::string& str)
    {
        for(char c : str)
            tracker_.onKeyEvent(makeKeyEvent(c));
    }
};
using CheatCommandTrackerFixture1P = CheatCommandTrackerFixture<1>;
using CheatCommandTrackerFixture2P = CheatCommandTrackerFixture<2>;
} // namespace

BOOST_FIXTURE_TEST_CASE(CheatModeIsOffByDefault, CheatCommandTrackerFixture1P)
{
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CheatModeCanBeTurnedOn, CheatCommandTrackerFixture1P)
{
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == true);
}

BOOST_FIXTURE_TEST_CASE(CheatModeCannotBeTurnedOn_InMultiplayer, CheatCommandTrackerFixture2P)
{
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CheatModeCanBeTurnedOff, CheatCommandTrackerFixture1P)
{
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == true);
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CheatModeCanBeTurnedOnAndOffRepeatedly, CheatCommandTrackerFixture1P)
{
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == true);
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == true);
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CheatModeIsNotTurnedOn_WhenIncomplete, CheatCommandTrackerFixture1P)
{
    trackString("winte");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CheatModeIsNotTurnedOn_WhenInterruptedByAnotherKeyType, CheatCommandTrackerFixture1P)
{
    trackString("win");
    tracker_.onKeyEvent(makeKeyEvent(KeyType::F10));
    trackString("ter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CheatModeIsNotTurnedOn_WhenInterruptedByAnotherLetter, CheatCommandTrackerFixture1P)
{
    trackString("wainter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CheatModeIsNotTurnedOn_WhenOrderOfCharactersIsWrong, CheatCommandTrackerFixture1P)
{
    trackString("winetr");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CheatModeIsNotTurnedOn_WhenOrderOfCharactersIsWrong_Wraparound, CheatCommandTrackerFixture1P)
{
    trackString("rwinte");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CheatModeIsNotTurnedOn_WhenACharacterIsRepeated, CheatCommandTrackerFixture1P)
{
    trackString("winnter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CheatModeIsTurnedOn_WhenTheFirstCharacterIsRepeated, CheatCommandTrackerFixture1P)
{
    trackString("wwwinter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == true);
}

BOOST_FIXTURE_TEST_CASE(CheatModeIsTurnedOn_EvenWhenWrongInputsWereProvidedBefore, CheatCommandTrackerFixture1P)
{
    trackString("www");
    auto ke = makeKeyEvent('1');
    ke.alt = true;
    tracker_.onKeyEvent(ke);
    trackString("interwitter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == false);
    trackString("winter");
    BOOST_TEST_REQUIRE(cheats_.isCheatModeOn() == true);
}

BOOST_AUTO_TEST_SUITE_END()
