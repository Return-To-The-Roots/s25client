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

#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "ai/AIPlayer.h"
#include "ai/aijh/AIPlayerJH.h"
#include "buildings/noBuilding.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "factories/AIFactory.h"
#include "factories/BuildingFactory.h"
#include "notifications/NodeNote.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noTree.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameData/BuildingProperties.h"
#include <boost/test/unit_test.hpp>
#include <memory>
#include <set>

// We need border land
using BiggerWorldWithGCExecution = WorldWithGCExecution<1, 24, 22>;

template<class T_Col>
inline bool containsBldType(const T_Col& collection, BuildingType type)
{
    return std::find_if(collection.begin(), collection.end(),
                        [type](const noBaseBuilding* bld) { return bld->GetBuildingType() == type; })
           != collection.end();
}

inline bool playerHasBld(const GamePlayer& player, BuildingType type)
{
    const BuildingRegister& blds = player.GetBuildingRegister();
    if(containsBldType(blds.GetBuildingSites(), type))
        return true;
    if(BuildingProperties::IsMilitary(type))
        return containsBldType(blds.GetMilitaryBuildings(), type);
    if(BuildingProperties::IsWareHouse(type)) // Includes harbors
        return containsBldType(blds.GetStorehouses(), type);
    return !blds.GetBuildings(type).empty();
}

// Note game command execution is emulated to be like the ones send via network:
// Run "Network Frame" then execute GCs from last NWF
// Also use "HARD" AI for faster execution
BOOST_AUTO_TEST_SUITE(AI)

BOOST_FIXTURE_TEST_CASE(PlayerHasBld_IsCorrect, WorldWithGCExecution<1>)
{
    const GamePlayer& player = world.GetPlayer(curPlayer);
    BOOST_TEST(playerHasBld(player, BuildingType::Headquarters));
    MapPoint pos = hqPos;
    for(const auto bld : {BuildingType::Woodcutter, BuildingType::Barracks, BuildingType::Storehouse})
    {
        pos = world.MakeMapPoint(pos + Position(2, 0));
        BOOST_TEST_INFO(bld);
        BOOST_TEST(!playerHasBld(player, bld));
        BuildingFactory::CreateBuilding(world, bld, pos, player.GetPlayerId(), Nation::Romans);
        BOOST_TEST_INFO(bld);
        BOOST_TEST(playerHasBld(player, bld));
    }
}

BOOST_FIXTURE_TEST_CASE(KeepBQUpdated, BiggerWorldWithGCExecution)
{
    // Place some trees to reduce BQ at some points
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        if(pt.x % 4 == 0 && pt.y % 2 == 0 && world.GetNode(pt).bq == BuildingQuality::Castle
           && world.CalcDistance(pt, hqPos) > 6)
            world.SetNO(pt, new noTree(pt, 0, 3));
    }
    world.InitAfterLoad();

    auto ai = AIFactory::Create(AI::Info(AI::Type::Default, AI::Level::Hard), curPlayer, world);
    const AIJH::AIPlayerJH& aijh = static_cast<AIJH::AIPlayerJH&>(*ai);

    const auto assertBqEqualOnWholeMap = [this, &aijh](const unsigned lineNr) {
        BOOST_TEST_CONTEXT("Line #" << lineNr)
        RTTR_FOREACH_PT(MapPoint, world.GetSize())
        {
            BOOST_TEST_INFO(pt);
            BOOST_TEST_REQUIRE(this->world.GetBQ(pt, curPlayer) == aijh.GetAINode(pt).bq);
        }
    };
    const auto assertBqEqualAround = [this, &aijh](const unsigned lineNr, MapPoint pt, unsigned radius) {
        BOOST_TEST_CONTEXT("Line #" << lineNr)
        world.CheckPointsInRadius(
          pt, radius,
          [&](const MapPoint curPt, unsigned) {
              BOOST_TEST_INFO(curPt);
              BOOST_TEST_REQUIRE(this->world.GetBQ(curPt, curPlayer) == aijh.GetAINode(curPt).bq);
              return false;
          },
          true);
    };

    // 100GFs for initialization
    for(unsigned gf = 0; gf < 100; ++gf)
    {
        em.ExecuteNextGF();
        ai->RunGF(em.GetCurrentGF(), true);
    }
    assertBqEqualOnWholeMap(__LINE__);

    // Set and destroy flag everywhere possible
    std::vector<MapPoint> possibleFlagNodes;
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        if(world.GetBQ(pt, curPlayer) != BuildingQuality::Nothing && !world.IsFlagAround(pt))
            possibleFlagNodes.push_back(pt);
    }
    for(const MapPoint flagPos : possibleFlagNodes)
    {
        this->SetFlag(flagPos);
        BOOST_REQUIRE(world.GetSpecObj<noFlag>(flagPos));
        em.ExecuteNextGF();
        ai->RunGF(em.GetCurrentGF(), true);
        assertBqEqualAround(__LINE__, flagPos, 3);

        this->DestroyFlag(flagPos);
        BOOST_REQUIRE(!world.GetSpecObj<noFlag>(flagPos));
        em.ExecuteNextGF();
        ai->RunGF(em.GetCurrentGF(), true);
        assertBqEqualAround(__LINE__, flagPos, 3);
    }

    // Build road
    const MapPoint flagPos = world.MakeMapPoint(world.GetNeighbour(hqPos, Direction::SOUTHEAST) + Position(4, 0));
    this->BuildRoad(world.GetNeighbour(hqPos, Direction::SOUTHEAST), false, std::vector<Direction>(4, Direction::EAST));
    BOOST_REQUIRE(world.GetSpecObj<noFlag>(flagPos)->GetRoute(Direction::WEST));
    em.ExecuteNextGF();
    ai->RunGF(em.GetCurrentGF(), true);
    assertBqEqualAround(__LINE__, flagPos, 6);

    // Destroy road and flag
    this->DestroyFlag(flagPos);
    em.ExecuteNextGF();
    ai->RunGF(em.GetCurrentGF(), true);
    assertBqEqualAround(__LINE__, flagPos, 6);

    // Build building
    const MapPoint bldPos = world.MakeMapPoint(hqPos + Position(5, 0));
    this->SetBuildingSite(bldPos, BuildingType::Barracks);
    BOOST_REQUIRE(world.GetSpecObj<noBuildingSite>(bldPos));
    em.ExecuteNextGF();
    ai->RunGF(em.GetCurrentGF(), true);
    assertBqEqualAround(__LINE__, bldPos, 6);

    this->BuildRoad(world.GetNeighbour(bldPos, Direction::SOUTHEAST), false,
                    std::vector<Direction>(5, Direction::WEST));
    em.ExecuteNextGF();
    ai->RunGF(em.GetCurrentGF(), true);
    RTTR_EXEC_TILL(2000, world.GetSpecObj<noBuilding>(bldPos));
    em.ExecuteNextGF();
    ai->RunGF(em.GetCurrentGF(), false);
    assertBqEqualOnWholeMap(__LINE__);

    // Gain land
    const nobMilitary* bld = world.GetSpecObj<nobMilitary>(bldPos);
    for(unsigned i = 0; i < 500; i++)
    {
        em.ExecuteNextGF();
        ai->RunGF(em.GetCurrentGF(), false);
        if(bld->GetNumTroops() > 0)
            break;
    }
    BOOST_REQUIRE(bld->GetNumTroops() > 0);
    assertBqEqualOnWholeMap(__LINE__);

    // Move the boundary by one node
    std::set<MapPoint, MapPointLess> outerBoundaryNodes;
    std::vector<MapPoint> borderNodes;
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        if(world.IsBorderNode(pt, curPlayer + 1))
        {
            borderNodes.push_back(pt);
            world.CheckPointsInRadius(
              pt, 1,
              [&outerBoundaryNodes, &world = this->world, curPlayer = this->curPlayer](const MapPoint curPt, unsigned) {
                  if(world.GetNode(curPt).owner != curPlayer + 1)
                      outerBoundaryNodes.insert(curPt);
                  return false;
              },
              false);
        }
    }
    // Once to outside
    for(const MapPoint pt : outerBoundaryNodes)
        world.SetOwner(pt, curPlayer + 1);
    world.RecalcBorderStones(Position(0, 0), Extent(world.GetSize()));
    for(const MapPoint pt : outerBoundaryNodes)
        world.GetNotifications().publish(NodeNote(NodeNote::Owner, pt));
    em.ExecuteNextGF();
    ai->RunGF(em.GetCurrentGF(), false);
    assertBqEqualOnWholeMap(__LINE__);

    // And back
    for(const MapPoint pt : outerBoundaryNodes)
        world.SetOwner(pt, 0);
    world.RecalcBorderStones(Position(0, 0), Extent(world.GetSize()));
    for(const MapPoint pt : outerBoundaryNodes)
        world.GetNotifications().publish(NodeNote(NodeNote::Owner, pt));
    em.ExecuteNextGF();
    ai->RunGF(em.GetCurrentGF(), false);
    assertBqEqualOnWholeMap(__LINE__);

    // And once to inside
    for(const MapPoint pt : borderNodes)
        world.SetOwner(pt, 0);
    world.RecalcBorderStones(Position(0, 0), Extent(world.GetSize()));
    for(const MapPoint pt : borderNodes)
        world.GetNotifications().publish(NodeNote(NodeNote::Owner, pt));
    em.ExecuteNextGF();
    ai->RunGF(em.GetCurrentGF(), false);
    assertBqEqualOnWholeMap(__LINE__);
}

BOOST_FIXTURE_TEST_CASE(BuildWoodIndustry, WorldWithGCExecution<1>)
{
    // Place a few trees
    for(const MapPoint& pt : world.GetPointsInRadius(hqPos + MapPoint(4, 0), 2))
    {
        if(!world.GetNode(pt).obj)
            world.SetNO(pt, new noTree(pt, 0, 3));
    }
    world.InitAfterLoad();

    const GamePlayer& player = world.GetPlayer(curPlayer);
    auto ai = AIFactory::Create(AI::Info(AI::Type::Default, AI::Level::Hard), curPlayer, world);
    // Build a woodcutter, sawmill and forester at some point
    for(unsigned gf = 0; gf < 2000;)
    {
        std::vector<gc::GameCommandPtr> aiGcs = ai->FetchGameCommands();
        for(unsigned i = 0; i < 5; i++, gf++)
        {
            em.ExecuteNextGF();
            ai->RunGF(em.GetCurrentGF(), i == 0);
        }
        for(gc::GameCommandPtr& gc : aiGcs)
        {
            gc->Execute(world, curPlayer);
        }
        if(playerHasBld(player, BuildingType::Sawmill) && playerHasBld(player, BuildingType::Woodcutter)
           && playerHasBld(player, BuildingType::Forester))
            break;
    }
    BOOST_REQUIRE(playerHasBld(player, BuildingType::Sawmill));
    BOOST_REQUIRE(playerHasBld(player, BuildingType::Woodcutter));
    BOOST_REQUIRE(playerHasBld(player, BuildingType::Forester));
}

BOOST_FIXTURE_TEST_CASE(ExpandWhenNoSpace, BiggerWorldWithGCExecution)
{
    const GamePlayer& player = world.GetPlayer(curPlayer);
    // No space for saw mill due to altitude diff of 3 in range 2 -> Huts only
    for(unsigned y = 0; y < world.GetHeight(); y += 2)
    {
        for(unsigned x = 0; x < world.GetWidth(); x += 2)
            world.ChangeAltitude(MapPoint(x, y), 13);
    }
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        BOOST_REQUIRE_LE(world.GetBQ(pt, curPlayer), BuildingQuality::Hut);
    }
    auto ai = AIFactory::Create(AI::Info(AI::Type::Default, AI::Level::Hard), curPlayer, world);
    const std::list<noBuildingSite*>& bldSites = player.GetBuildingRegister().GetBuildingSites();
    // Can't build sawmill -> Expand anyway
    for(unsigned gf = 0; gf < 2000;)
    {
        std::vector<gc::GameCommandPtr> aiGcs = ai->FetchGameCommands();
        for(unsigned i = 0; i < 5; i++, gf++)
        {
            em.ExecuteNextGF();
            ai->RunGF(em.GetCurrentGF(), i == 0);
        }
        for(gc::GameCommandPtr& gc : aiGcs)
        {
            gc->Execute(world, curPlayer);
        }
        if(containsBldType(bldSites, BuildingType::Barracks) || containsBldType(bldSites, BuildingType::Guardhouse))
            break;
    }
    BOOST_REQUIRE(containsBldType(bldSites, BuildingType::Barracks)
                  || containsBldType(bldSites, BuildingType::Guardhouse));
}

BOOST_AUTO_TEST_SUITE_END()
