// Copyright (C) 2024-2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Cheats.h"
#include "GameInterface.h"
#include "GamePlayer.h"
#include "buildings/nobHQ.h"
#include "desktops/dskGameInterface.h"
#include "factories/GameCommandFactory.h"
#include "network/GameClient.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "gameData/MilitaryConsts.h"
#include <turtle/mock.hpp>

BOOST_AUTO_TEST_SUITE(CheatsTests)

namespace {

MOCK_BASE_CLASS(MockGCFactory, GameCommandFactory)
{
    MOCK_METHOD(AddGC, 1, bool(gc::GameCommandPtr gc)); // LCOV_EXCL_LINE
};

constexpr auto numPlayers = 3;
constexpr auto worldWidth = 64;
constexpr auto worldHeight = 64;
struct CheatsFixture : WorldFixture<CreateEmptyWorld, numPlayers, worldWidth, worldHeight>
{
    CheatsFixture() : cheats(world, gcFactory), viewer(0, world)
    {
        p2.ps = PlayerState::AI;
        p3.ps = PlayerState::AI;
    }

    const GameWorldViewer& getViewer() const { return viewer; }

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
        for(auto* bld : player.GetBuildingRegister().GetStorehouses())
        {
            if(bld->GetBuildingType() == BuildingType::Headquarters)
                ret.push_back(static_cast<nobHQ*>(bld));
        }
        return ret;
    }

    auto countAllBuildings(const GamePlayer& player)
    {
        unsigned ret = 0;
        for(auto b : player.GetBuildingRegister().GetBuildingNums().buildings)
            ret += b;
        return ret;
    }

    MockGCFactory gcFactory;
    Cheats cheats;

private:
    GameWorldViewer viewer;
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

BOOST_FIXTURE_TEST_CASE(TurningCheatModeOffDisablesAllCheats, CheatsFixture)
{
    cheats.toggleCheatMode();
    cheats.toggleAllVisible();
    cheats.toggleAllBuildingsEnabled();
    cheats.toggleShowEnemyProductivityOverlay();
    BOOST_TEST_REQUIRE(cheats.isAllVisible() == true);
    BOOST_TEST_REQUIRE(cheats.areAllBuildingsEnabled() == true);
    BOOST_TEST_REQUIRE(cheats.shouldShowEnemyProductivityOverlay() == true);

    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.isAllVisible() == false);
    BOOST_TEST_REQUIRE(cheats.areAllBuildingsEnabled() == false);
    BOOST_TEST_REQUIRE(cheats.shouldShowEnemyProductivityOverlay() == false);
    // testing toggleHumanAIPlayer would require GameClient::state==Loaded, which is guaranteed in code (because Cheats
    // only exist after the game is loaded) but is not the case in tests - skipping
}

namespace {
MOCK_BASE_CLASS(MockGameInterface, GameInterface)
{
    // LCOV_EXCL_START
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
    // clang-format off
    MOCK_METHOD(GI_GetCheats, 0, Cheats&(void))
    // clang-format on
    // LCOV_EXCL_STOP
};
} // namespace

BOOST_FIXTURE_TEST_CASE(CanToggleAllVisible_IfCheatModeIsOn, CheatsFixture)
{
    MockGameInterface mgi;
    MOCK_EXPECT(mgi.GI_GetCheats).returns(std::ref(this->cheats));
    MOCK_EXPECT(mgi.GI_UpdateMapVisibility).exactly(3); // how many toggles should actually occur
    world.SetGameInterface(&mgi);

    MapPoint farawayPos = p1HQPos;
    farawayPos.x += 20;

    const auto& viewer = getViewer();

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
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == false);
    cheats.toggleAllVisible();
    BOOST_TEST_REQUIRE((viewer.GetVisibility(farawayPos) == Visibility::Visible) == true);
}

BOOST_FIXTURE_TEST_CASE(CanToggleAllBuildingsEnabled_AndShowEnemyProductivityOverlay_IfCheatModeIsOn, CheatsFixture)
{
    BOOST_TEST_REQUIRE(cheats.areAllBuildingsEnabled() == false);
    BOOST_TEST_REQUIRE(cheats.shouldShowEnemyProductivityOverlay() == false);

    // Can't change anything if cheat mode is off
    BOOST_TEST_REQUIRE(cheats.isCheatModeOn() == false);
    cheats.toggleAllBuildingsEnabled();
    BOOST_TEST_REQUIRE(cheats.areAllBuildingsEnabled() == false);
    cheats.toggleShowEnemyProductivityOverlay();
    BOOST_TEST_REQUIRE(cheats.areAllBuildingsEnabled() == false);
    BOOST_TEST_REQUIRE(cheats.shouldShowEnemyProductivityOverlay() == false);

    cheats.toggleCheatMode();
    cheats.toggleAllBuildingsEnabled();
    BOOST_TEST_REQUIRE(cheats.areAllBuildingsEnabled() == true);
    cheats.toggleShowEnemyProductivityOverlay();
    BOOST_TEST_REQUIRE(cheats.shouldShowEnemyProductivityOverlay() == true);

    cheats.toggleAllBuildingsEnabled();
    BOOST_TEST_REQUIRE(cheats.areAllBuildingsEnabled() == false);
    cheats.toggleShowEnemyProductivityOverlay();
    BOOST_TEST_REQUIRE(cheats.shouldShowEnemyProductivityOverlay() == false);

    cheats.toggleShowEnemyProductivityOverlay();
    BOOST_TEST_REQUIRE(cheats.shouldShowEnemyProductivityOverlay() == true);
    cheats.toggleAllBuildingsEnabled();
    BOOST_TEST_REQUIRE(cheats.areAllBuildingsEnabled() == true);
}

BOOST_FIXTURE_TEST_CASE(AllBuildingsAreEnabled_WhenCheatModeIsOn, CheatsFixture)
{
    MockGameInterface mgi;
    MOCK_EXPECT(mgi.GI_GetCheats).returns(std::ref(this->cheats));
    world.SetGameInterface(&mgi);
    cheats.toggleCheatMode();
    GamePlayer& p1 = world.GetPlayer(0);
    const auto bld = BuildingType::Brewery;
    p1.DisableBuilding(bld);
    BOOST_TEST_REQUIRE(p1.IsBuildingEnabled(bld) == false);
    cheats.toggleAllBuildingsEnabled();
    BOOST_TEST_REQUIRE(p1.IsBuildingEnabled(bld) == true);
    cheats.toggleAllBuildingsEnabled();
    BOOST_TEST_REQUIRE(p1.IsBuildingEnabled(bld) == false);
    cheats.toggleAllBuildingsEnabled();
    BOOST_TEST_REQUIRE(p1.IsBuildingEnabled(bld) == true);
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(p1.IsBuildingEnabled(bld) == false);
}

BOOST_FIXTURE_TEST_CASE(CannotPlaceCheatBuildingWithinOwnedTerritory, CheatsFixture)
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

BOOST_FIXTURE_TEST_CASE(CannotPlaceCheatBuildingAtTerritoryBorderOrOneNodeFurther, CheatsFixture)
{
    cheats.toggleCheatMode();

    MapPoint border = p1HQPos;
    border.x += HQ_RADIUS;
    BOOST_TEST_REQUIRE(cheats.canPlaceCheatBuilding(border) == false);

    MapPoint nodeBeyondBorder = border;
    ++border.x;
    BOOST_TEST_REQUIRE(cheats.canPlaceCheatBuilding(nodeBeyondBorder) == false);
}

BOOST_FIXTURE_TEST_CASE(CanPlaceCheatBuildingOutsideOwnedTerritory, CheatsFixture)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(cheats.canPlaceCheatBuilding(unownedPt) == true);
}

BOOST_FIXTURE_TEST_CASE(CannotPlaceCheatBuilding_IfCheatModeIsNotOn, CheatsFixture)
{
    BOOST_TEST_REQUIRE(cheats.canPlaceCheatBuilding(unownedPt) == false);
}

BOOST_FIXTURE_TEST_CASE(PlacesHQAsACheatBuilding, CheatsFixture)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(countHQs(p1) == 1);
    cheats.placeCheatBuilding(unownedPt, p1);
    BOOST_TEST_REQUIRE(countHQs(p1) == 2);
}

BOOST_FIXTURE_TEST_CASE(DoesNotPlaceHQAsACheatBuilding_IfCheatModeIsNotOn, CheatsFixture)
{
    BOOST_TEST_REQUIRE(countHQs(p1) == 1);
    cheats.placeCheatBuilding(unownedPt, p1);
    BOOST_TEST_REQUIRE(countHQs(p1) == 1);
}

BOOST_FIXTURE_TEST_CASE(CanPlaceCheatBuildingForAnyPlayer, CheatsFixture)
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

BOOST_FIXTURE_TEST_CASE(CheatBuildingHasTheSameNation, CheatsFixture)
{
    cheats.toggleCheatMode();
    cheats.placeCheatBuilding(unownedPt, p1);
    for(auto* bld : getHQs(p1))
        BOOST_TEST_REQUIRE((bld->GetNation() == p1.nation) == true);
}

BOOST_FIXTURE_TEST_CASE(CheatBuildingIsATent_IfPrimaryHQIsATent, CheatsFixture)
{
    p1.SetHQIsTent(true);
    cheats.toggleCheatMode();
    cheats.placeCheatBuilding(unownedPt, p1);
    for(auto* bld : getHQs(p1))
        BOOST_TEST_REQUIRE(static_cast<nobHQ*>(bld)->IsTent() == true);
}

BOOST_FIXTURE_TEST_CASE(CheatBuildingIsNotATent_IfPrimaryHQIsNotATent, CheatsFixture)
{
    p1.SetHQIsTent(false);
    cheats.toggleCheatMode();
    cheats.placeCheatBuilding(unownedPt, p1);
    for(auto* bld : getHQs(p1))
        BOOST_TEST_REQUIRE(static_cast<nobHQ*>(bld)->IsTent() == false);
}

BOOST_FIXTURE_TEST_CASE(CanToggleResourcesToRevealSuccessively, CheatsFixture)
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

BOOST_FIXTURE_TEST_CASE(DestroyBuildingsOfGivenPlayer, CheatsFixture)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 1);
    BOOST_TEST_REQUIRE(countAllBuildings(p2) == 1);
    cheats.destroyBuildings({0});
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 0);
    BOOST_TEST_REQUIRE(countAllBuildings(p2) == 1);
}

BOOST_FIXTURE_TEST_CASE(DestroyBuildingsOfGivenPlayers, CheatsFixture)
{
    cheats.toggleCheatMode();
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 1);
    BOOST_TEST_REQUIRE(countAllBuildings(p2) == 1);
    cheats.destroyBuildings({0, 1});
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 0);
    BOOST_TEST_REQUIRE(countAllBuildings(p2) == 0);
}

BOOST_FIXTURE_TEST_CASE(DestroyBuildingsOfAIPlayers, CheatsFixture)
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

BOOST_FIXTURE_TEST_CASE(CannotDestroyBuildings_IfCheatModeIsNotOn, CheatsFixture)
{
    p1.ps = PlayerState::AI;
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 1);
    cheats.destroyBuildings({0});
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 1);
    cheats.destroyAllAIBuildings();
    BOOST_TEST_REQUIRE(countAllBuildings(p1) == 1);
}

BOOST_AUTO_TEST_SUITE_END()
