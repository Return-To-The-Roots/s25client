// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Cheats.h"
#include "GamePlayer.h"
#include "desktops/dskGameInterface.h"
#include "network/GameClient.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include <turtle/mock.hpp>

namespace {
MOCK_BASE_CLASS(MockGameInterface, GameInterface)
{
    MOCK_METHOD(GI_PlayerDefeated, 1)
    MOCK_METHOD(GI_UpdateMinimap, 1)
    MOCK_METHOD(GI_FlagDestroyed, 1)
    MOCK_METHOD(GI_TreatyOfAllianceChanged, 1)
    MOCK_METHOD(GI_UpdateMapVisibility, 0)
    MOCK_METHOD(GI_Winner, 1)
    MOCK_METHOD(GI_TeamWinner, 1)
    MOCK_METHOD(GI_StartRoadBuilding, 2)
    MOCK_METHOD(GI_CancelRoadBuilding, 0)
    MOCK_METHOD(GI_BuildRoad, 0)
};

constexpr auto worldWidth = 64;
constexpr auto worldHeight = 64;
struct CheatWorldFixture : WorldFixture<CreateEmptyWorld, 2, worldWidth, worldHeight>
{
    Cheats& cheats = world.GetCheats();

    dskGameInterface gameDesktop{game, 0, 0, 0};
    const GameWorldViewer& viewer = gameDesktop.GetView().GetViewer();

    MockGameInterface mgi;

    GamePlayer& getPlayer(unsigned id) { return world.GetPlayer(id); }
    GamePlayer& p1 = getPlayer(0);
    GamePlayer& p2 = getPlayer(1);

    CheatWorldFixture()
    {
        p2.ps = PlayerState::AI;
        world.SetGameInterface(&mgi);
    }
};
} // namespace

BOOST_FIXTURE_TEST_CASE(CheatModeIsOffByDefault, CheatWorldFixture)
{
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CanToggleCheatModeOn, CheatWorldFixture)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == true);
}

BOOST_FIXTURE_TEST_CASE(CannotToggleCheatModeOn_IfMultiplayer, CheatWorldFixture)
{
    p2.ps = PlayerState::Occupied;
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CanToggleCheatModeOnAndOff, CheatWorldFixture)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == true);
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == false);
}

BOOST_FIXTURE_TEST_CASE(CanToggleCheatModeOnAndOffRepeatedly, CheatWorldFixture)
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

BOOST_FIXTURE_TEST_CASE(CanToggleAllVisible_IfCheatModeIsOn, CheatWorldFixture)
{
    const MapPoint p1HQPos = world.GetPlayer(0).GetHQPos();
    MapPoint farawayPos = p1HQPos;
    farawayPos.x += 20;
    BOOST_TEST_REQUIRE((viewer.GetVisibility(p1HQPos) == Visibility::Visible) == true);
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == false);
    cheats.toggleCheatMode();
    MOCK_EXPECT(mgi.GI_UpdateMapVisibility).once();
    cheats.toggleAllVisible();
    BOOST_TEST_REQUIRE((viewer.GetVisibility(p1HQPos) == Visibility::Visible) == true);
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == true);
    MOCK_EXPECT(mgi.GI_UpdateMapVisibility).once();
    cheats.toggleAllVisible();
    BOOST_TEST_REQUIRE((viewer.GetVisibility(p1HQPos) == Visibility::Visible) == true);
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == false);
}

BOOST_FIXTURE_TEST_CASE(CannotToggleAllVisible_IfCheatModeIsNotOn, CheatWorldFixture)
{
    MOCK_EXPECT(mgi.GI_UpdateMapVisibility).never();
    const MapPoint p1HQPos = world.GetPlayer(0).GetHQPos();
    MapPoint farawayPos = p1HQPos;
    farawayPos.x += 20;
    BOOST_TEST_REQUIRE((viewer.GetVisibility(p1HQPos) == Visibility::Visible) == true);
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == false);
    cheats.toggleAllVisible();
    BOOST_TEST_REQUIRE((viewer.GetVisibility(p1HQPos) == Visibility::Visible) == true);
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == false);
}
