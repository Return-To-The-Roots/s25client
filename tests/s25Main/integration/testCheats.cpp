// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Cheats.h"
#include "GamePlayer.h"
#include "desktops/dskGameInterface.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include <turtle/mock.hpp>

BOOST_AUTO_TEST_SUITE(CheatsTests)

namespace {
constexpr auto numPlayers = 1;
constexpr auto worldWidth = 64;
constexpr auto worldHeight = 64;
struct CheatsFixture : WorldFixture<CreateEmptyWorld, numPlayers, worldWidth, worldHeight>
{
    CheatsFixture()
        : cheats{gameDesktop.GI_GetCheats()}, viewer{gameDesktop.GetView().GetViewer()}, p1HQPos{p1.GetHQPos()}
    {}

    dskGameInterface gameDesktop{game, nullptr, 0, false};
    Cheats& cheats;
    const GameWorldViewer& viewer;

    GamePlayer& getPlayer(unsigned id) { return world.GetPlayer(id); }
    GamePlayer& p1 = getPlayer(0);
    const MapPoint p1HQPos;
};
} // namespace

BOOST_FIXTURE_TEST_CASE(CanToggleCheatModeOnAndOffRepeatedly, CheatsFixture)
{
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == false); // initially off
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == true);
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == false);
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == true);
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == false);
}

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
    MOCK_METHOD(GI_GetCheats, 0)
};
} // namespace

BOOST_FIXTURE_TEST_CASE(CanToggleAllVisible_IfCheatModeIsOn_ButOnlyDisableAllVisible_IfCheatModeIsNotOn, CheatsFixture)
{
    MockGameInterface mgi;
    MOCK_EXPECT(mgi.GI_GetCheats).returns(std::ref(gameDesktop.GI_GetCheats()));
    MOCK_EXPECT(mgi.GI_UpdateMapVisibility).exactly(4); // because the actual toggling should occur 4 times
    world.SetGameInterface(&mgi);

    MapPoint farawayPos = p1HQPos;
    farawayPos.x += 20;

    // initially farawayPos is not visible
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == false);

    cheats.toggleAllVisible();
    // still not visible - cheat mode is not on
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == false);

    cheats.toggleCheatMode();
    cheats.toggleAllVisible();
    // now visible - cheat mode is on
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == true);

    cheats.toggleAllVisible();
    // now not visible
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == false);

    cheats.toggleAllVisible();
    // visible again
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == true);

    cheats.toggleCheatMode();
    cheats.toggleAllVisible();
    // again not visible, despite cheat mode being off
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == false);
}

BOOST_AUTO_TEST_SUITE_END()
