// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Cheats.h"
#include "GamePlayer.h"
#include "buildings/nobHQ.h"
#include "desktops/dskGameInterface.h"
#include "network/GameClient.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "gameData/MilitaryConsts.h"
#include <turtle/mock.hpp>

namespace {
constexpr auto worldWidth = 64;
constexpr auto worldHeight = 64;
struct CheatWorldFixture : WorldFixture<CreateEmptyWorld, 3, worldWidth, worldHeight>
{
    CheatWorldFixture()
    {
        p2.ps = PlayerState::AI;
        p3.ps = PlayerState::AI;
    }

    Cheats& cheats = world.GetCheats();

    dskGameInterface gameDesktop{game, 0, 0, 0};
    const GameWorldViewer& viewer = gameDesktop.GetView().GetViewer();

    GamePlayer& getPlayer(unsigned id) { return world.GetPlayer(id); }
    GamePlayer& p1 = getPlayer(0);
    GamePlayer& p2 = getPlayer(1);
    GamePlayer& p3 = getPlayer(2);

    const MapPoint p1HQPos = p1.GetHQPos();
    const MapPoint p2HQPos = p2.GetHQPos();

    MapPoint unownedPt = {static_cast<MapCoord>(p1HQPos.x + HQ_RADIUS + 2), p1HQPos.y};

    auto countHQs(const GamePlayer& player)
    {
        return player.GetBuildingRegister().GetBuildingNums().buildings[BuildingType::Headquarters];
    }
    auto getHQs(const GamePlayer& player)
    {
        std::vector<nobHQ*> ret;
        for(auto bld : player.GetBuildingRegister().GetStorehouses())
            if(bld->GetBuildingType() == BuildingType::Headquarters)
                ret.push_back(static_cast<nobHQ*>(bld));
        return ret;
    }

    auto countAllBuildings(const GamePlayer& player)
    {
        unsigned ret = 0;
        for(auto b : player.GetBuildingRegister().GetBuildingNums().buildings)
            ret += b;
        return ret;
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

BOOST_FIXTURE_TEST_CASE(CanToggleAllVisible_IfCheatModeIsOn, CheatWorldFixture)
{
    MockGameInterface mgi;
    world.SetGameInterface(&mgi);

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
    MockGameInterface mgi;
    world.SetGameInterface(&mgi);

    MOCK_EXPECT(mgi.GI_UpdateMapVisibility).never();
    MapPoint farawayPos = p1HQPos;
    farawayPos.x += 20;
    BOOST_TEST_REQUIRE((viewer.GetVisibility(p1HQPos) == Visibility::Visible) == true);
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == false);
    cheats.toggleAllVisible();
    BOOST_TEST_REQUIRE((viewer.GetVisibility(p1HQPos) == Visibility::Visible) == true);
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == false);
}

BOOST_FIXTURE_TEST_CASE(CannotPlaceCheatBuildingWithinOwnedTerritory, CheatWorldFixture)
{
    cheats.toggleCheatMode();

    MapPoint p1territory = p1HQPos;
    p1territory.x += 3;
    p1territory.y += 3;
    BOOST_TEST_REQUIRE(cheats.canPlaceCheatBuilding(p1territory) == false);

    MapPoint p2territory = p2HQPos;
    p2territory.x += 3;
    p2territory.y += 3;
    BOOST_TEST_REQUIRE(cheats.canPlaceCheatBuilding(p2territory) == false);
}

BOOST_FIXTURE_TEST_CASE(CannotPlaceCheatBuildingAtTerritoryBorderOrOneNodeFurther, CheatWorldFixture)
{
    cheats.toggleCheatMode();

    MapPoint border = p1HQPos;
    border.x += HQ_RADIUS;
    BOOST_TEST_REQUIRE(cheats.canPlaceCheatBuilding(border) == false);

    MapPoint nodeBeyondBorder = border;
    ++border.x;
    BOOST_TEST_REQUIRE(cheats.canPlaceCheatBuilding(nodeBeyondBorder) == false);
}

BOOST_FIXTURE_TEST_CASE(CanPlaceCheatBuildingOutsideOwnedTerritory, CheatWorldFixture)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.canPlaceCheatBuilding(unownedPt) == true);
}

BOOST_FIXTURE_TEST_CASE(CannotPlaceCheatBuilding_IfCheatModeIsNotOn, CheatWorldFixture)
{
    BOOST_TEST_REQUIRE(cheats.canPlaceCheatBuilding(unownedPt) == false);
}

BOOST_FIXTURE_TEST_CASE(PlacesHQAsACheatBuilding, CheatWorldFixture)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(countHQs(p1) == 1);
    cheats.placeCheatBuilding(unownedPt, p1);
    BOOST_TEST_REQUIRE(countHQs(p1) == 2);
}

BOOST_FIXTURE_TEST_CASE(DoesNotPlaceHQAsACheatBuilding_IfCheatModeIsNotOn, CheatWorldFixture)
{
    BOOST_TEST_REQUIRE(countHQs(p1) == 1);
    cheats.placeCheatBuilding(unownedPt, p1);
    BOOST_TEST_REQUIRE(countHQs(p1) == 1);
}

BOOST_FIXTURE_TEST_CASE(CanPlaceCheatBuildingForAnyPlayer, CheatWorldFixture)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(countHQs(p1) == 1);
    BOOST_TEST_REQUIRE(countHQs(p2) == 1);
    cheats.placeCheatBuilding(unownedPt, p2);
    BOOST_TEST_REQUIRE(countHQs(p1) == 1);
    BOOST_TEST_REQUIRE(countHQs(p2) == 2);
    unownedPt.x -= 2 * (HQ_RADIUS + 2);
    cheats.placeCheatBuilding(unownedPt, p1);
    BOOST_TEST_REQUIRE(countHQs(p1) == 2);
    BOOST_TEST_REQUIRE(countHQs(p2) == 2);
}

BOOST_FIXTURE_TEST_CASE(CheatBuildingHasTheSameNation, CheatWorldFixture)
{
    cheats.toggleCheatMode();
    cheats.placeCheatBuilding(unownedPt, p1);
    for(auto bld : getHQs(p1))
        BOOST_TEST_REQUIRE((bld->GetNation() == p1.nation) == true);
}

BOOST_FIXTURE_TEST_CASE(CheatBuildingIsATent_IfPrimaryHQIsATent, CheatWorldFixture)
{
    p1.SetHQIsTent(true);
    cheats.toggleCheatMode();
    cheats.placeCheatBuilding(unownedPt, p1);
    for(auto bld : getHQs(p1))
        BOOST_TEST_REQUIRE(static_cast<nobHQ*>(bld)->IsTent() == true);
}

BOOST_FIXTURE_TEST_CASE(CheatBuildingIsNotATent_IfPrimaryHQIsNotATent, CheatWorldFixture)
{
    p1.SetHQIsTent(false);
    cheats.toggleCheatMode();
    cheats.placeCheatBuilding(unownedPt, p1);
    for(auto bld : getHQs(p1))
        BOOST_TEST_REQUIRE(static_cast<nobHQ*>(bld)->IsTent() == false);
}

BOOST_FIXTURE_TEST_CASE(CanToggleResourcesToRevealSuccessively, CheatWorldFixture)
{
    using RRM = Cheats::ResourceRevealMode;

    BOOST_CHECK(cheats.getResourceRevealMode() == RRM::Nothing);
    cheats.toggleCheatMode();
    BOOST_CHECK(cheats.getResourceRevealMode() == RRM::Nothing);
    cheats.toggleResourceRevealMode();
    BOOST_CHECK(cheats.getResourceRevealMode() == RRM::Ores);
    cheats.toggleResourceRevealMode();
    BOOST_CHECK(cheats.getResourceRevealMode() == RRM::Fish);
    cheats.toggleResourceRevealMode();
    BOOST_CHECK(cheats.getResourceRevealMode() == RRM::Water);
    cheats.toggleResourceRevealMode();
    BOOST_CHECK(cheats.getResourceRevealMode() == RRM::Nothing);
    cheats.toggleResourceRevealMode();
    BOOST_CHECK(cheats.getResourceRevealMode() == RRM::Ores);
    cheats.toggleCheatMode();
    BOOST_CHECK(cheats.getResourceRevealMode() == RRM::Nothing);
    cheats.toggleResourceRevealMode();
    cheats.toggleCheatMode();
    BOOST_CHECK(cheats.getResourceRevealMode() == RRM::Fish);
}

BOOST_FIXTURE_TEST_CASE(DestroyBuildingsOfGivenPlayer, CheatWorldFixture)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 1);
    BOOST_TEST_REQUIRE(countAllBuildings(p2) == 1);
    cheats.destroyBuildings({0});
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 0);
    BOOST_TEST_REQUIRE(countAllBuildings(p2) == 1);
}

BOOST_FIXTURE_TEST_CASE(DestroyBuildingsOfGivenPlayers, CheatWorldFixture)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 1);
    BOOST_TEST_REQUIRE(countAllBuildings(p2) == 1);
    cheats.destroyBuildings({0, 1});
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 0);
    BOOST_TEST_REQUIRE(countAllBuildings(p2) == 0);
}

BOOST_FIXTURE_TEST_CASE(DestroyBuildingsOfAIPlayers, CheatWorldFixture)
{
    cheats.toggleCheatMode();
    p2.ps = PlayerState::AI;
    p3.ps = PlayerState::AI;
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 1);
    BOOST_TEST_REQUIRE(countAllBuildings(p2) == 1);
    BOOST_TEST_REQUIRE(countAllBuildings(p3) == 1);
    cheats.destroyAllAIBuildings();
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 1);
    BOOST_TEST_REQUIRE(countAllBuildings(p2) == 0);
    BOOST_TEST_REQUIRE(countAllBuildings(p3) == 0);
}

BOOST_FIXTURE_TEST_CASE(CannotDestroyBuildings_IfCheatModeIsNotOn, CheatWorldFixture)
{
    p1.ps = PlayerState::AI;
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 1);
    cheats.destroyBuildings({0});
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 1);
    cheats.destroyAllAIBuildings();
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 1);
}
