// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Cheats.h"
#include "GamePlayer.h"
#include "network/GameClient.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"

namespace {
template<unsigned T_numPlayers>
struct CheatWorldFixture : WorldFixture<CreateEmptyWorld, T_numPlayers>
{
    Cheats cheats{world};
};

using CheatWorldFixture1P = CheatWorldFixture<1>;
using CheatWorldFixture2P = CheatWorldFixture<2>;
} // namespace

BOOST_FIXTURE_TEST_CASE(CheatModeIsOffByDefault, CheatWorldFixture1P)
{
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CanToggleCheatModeOn, CheatWorldFixture1P)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == true);
}

BOOST_FIXTURE_TEST_CASE(CannotToggleCheatModeOn_IfMultiplayer, CheatWorldFixture2P)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CanToggleCheatModeOnAndOff, CheatWorldFixture1P)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == true);
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CanToggleCheatModeOnAndOffRepeatedly, CheatWorldFixture1P)
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
