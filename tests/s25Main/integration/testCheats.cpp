// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Cheats.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"

BOOST_AUTO_TEST_SUITE(CheatsTests)

namespace {
template<unsigned T_numPlayers>
struct CheatsFixture : WorldFixture<CreateEmptyWorld, T_numPlayers>
{
    Cheats cheats{this->world};
};
using CheatsFixture1P = CheatsFixture<1>;
using CheatsFixture2P = CheatsFixture<2>;
} // namespace

BOOST_FIXTURE_TEST_CASE(CheatsAreAllowed_InSinglePlayer, CheatsFixture1P)
{
    BOOST_TEST_REQUIRE(cheats.areCheatsAllowed() == true);
}

BOOST_FIXTURE_TEST_CASE(CheatsAreNotAllowed_InMultiplayer, CheatsFixture2P)
{
    BOOST_TEST_REQUIRE(cheats.areCheatsAllowed() == false);
}

BOOST_FIXTURE_TEST_CASE(CheatModeIsOffByDefault, CheatsFixture1P)
{
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CanToggleCheatModeOn, CheatsFixture1P)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == true);
}

BOOST_FIXTURE_TEST_CASE(CanToggleCheatModeOnAndOff, CheatsFixture1P)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == true);
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CanToggleCheatModeOnAndOffRepeatedly, CheatsFixture1P)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == true);
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == false);
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == true);
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == false);
}

BOOST_AUTO_TEST_SUITE_END()
