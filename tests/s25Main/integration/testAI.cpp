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

#include "RttrForeachPt.h"
#include "ai/AIPlayer.h"
#include "ai/aijh/AIPlayerJH.h"
#include "buildings/noBuilding.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "factories/AIFactory.h"
#include "factories/BuildingFactory.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noTree.h"
#include "gameData/BuildingProperties.h"
#include <boost/test/unit_test.hpp>
#include <memory>

// We need border land
using BiggerWorldWithGCExecution = WorldWithGCExecution<1, 24, 22>;

template<class T_Col>
inline bool containsBldType(const T_Col& collection, BuildingType type)
{
    return std::find_if(collection.begin(), collection.end(), [type](const noBaseBuilding* bld) { return bld->GetBuildingType() == type; })
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
    BOOST_TEST(playerHasBld(player, BLD_HEADQUARTERS));
    MapPoint pos = hqPos;
    for(const auto bld : {BLD_WOODCUTTER, BLD_BARRACKS, BLD_STOREHOUSE})
    {
        pos = world.MakeMapPoint(pos + Position(2, 0));
        BOOST_TEST_INFO(bld);
        BOOST_TEST(!playerHasBld(player, bld));
        BuildingFactory::CreateBuilding(world, bld, pos, player.GetPlayerId(), NAT_ROMANS);
        BOOST_TEST_INFO(bld);
        BOOST_TEST(playerHasBld(player, bld));
    }
}

BOOST_FIXTURE_TEST_CASE(KeepBQUpdated, BiggerWorldWithGCExecution)
{
    auto ai = AIFactory::Create(AI::Info(AI::DEFAULT, AI::HARD), curPlayer, world);
    const AIJH::AIPlayerJH& aijh = static_cast<AIJH::AIPlayerJH&>(*ai);
    RTTR_FOREACH_PT(MapPoint, world.GetSize()) //-V807
    {
        BOOST_REQUIRE_EQUAL(world.GetBQ(pt, curPlayer), aijh.GetAINode(pt).bq);
    }

    // Set flag
    MapPoint flagPos = world.MakeMapPoint(world.GetNeighbour(hqPos, Direction::SOUTHEAST) + Position(4, 0));
    this->SetFlag(flagPos);
    BOOST_REQUIRE(world.GetSpecObj<noFlag>(flagPos));
    em.ExecuteNextGF();
    ai->RunGF(em.GetCurrentGF(), true);
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        BOOST_REQUIRE_EQUAL(world.GetBQ(pt, curPlayer), aijh.GetAINode(pt).bq);
    }

    // Build road
    this->BuildRoad(flagPos, false, std::vector<Direction>(4, Direction::WEST));
    BOOST_REQUIRE(world.GetSpecObj<noFlag>(flagPos)->GetRoute(Direction::WEST));
    em.ExecuteNextGF();
    ai->RunGF(em.GetCurrentGF(), true);
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        BOOST_REQUIRE_EQUAL(world.GetBQ(pt, curPlayer), aijh.GetAINode(pt).bq);
    }

    // Destroy road and flag
    this->DestroyFlag(flagPos);
    em.ExecuteNextGF();
    ai->RunGF(em.GetCurrentGF(), true);
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        BOOST_REQUIRE_EQUAL(world.GetBQ(pt, curPlayer), aijh.GetAINode(pt).bq);
    }

    // Build building
    MapPoint bldPos = world.MakeMapPoint(hqPos + Position(6, 0));
    this->SetBuildingSite(bldPos, BLD_BARRACKS);
    BOOST_REQUIRE(world.GetSpecObj<noBuildingSite>(bldPos));
    em.ExecuteNextGF();
    ai->RunGF(em.GetCurrentGF(), true);
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        BOOST_REQUIRE_EQUAL(world.GetBQ(pt, curPlayer), aijh.GetAINode(pt).bq);
    }
    this->BuildRoad(world.GetNeighbour(bldPos, Direction::SOUTHEAST), false, std::vector<Direction>(6, Direction::WEST));
    em.ExecuteNextGF();
    ai->RunGF(em.GetCurrentGF(), true);
    RTTR_EXEC_TILL(2000, world.GetSpecObj<noBuilding>(bldPos));
    em.ExecuteNextGF();
    ai->RunGF(em.GetCurrentGF(), false);
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        BOOST_REQUIRE_EQUAL(world.GetBQ(pt, curPlayer), aijh.GetAINode(pt).bq);
    }
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
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        BOOST_REQUIRE_EQUAL(world.GetBQ(pt, curPlayer), aijh.GetAINode(pt).bq);
    }
}

BOOST_FIXTURE_TEST_CASE(BuildWoodIndustry, WorldWithGCExecution<1>)
{
    // Place a few trees
    for(const MapPoint& pt : world.GetPointsInRadius(hqPos + MapPoint(4, 0), 2))
    {
        if(!world.GetNode(pt).obj)
            world.SetNO(pt, new noTree(pt, 0, 3));
    }
    const GamePlayer& player = world.GetPlayer(curPlayer);
    auto ai = AIFactory::Create(AI::Info(AI::DEFAULT, AI::HARD), curPlayer, world);
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
        if(playerHasBld(player, BLD_SAWMILL) && playerHasBld(player, BLD_WOODCUTTER) && playerHasBld(player, BLD_FORESTER))
            break;
    }
    BOOST_REQUIRE(playerHasBld(player, BLD_SAWMILL));
    BOOST_REQUIRE(playerHasBld(player, BLD_WOODCUTTER));
    BOOST_REQUIRE(playerHasBld(player, BLD_FORESTER));
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
        BOOST_REQUIRE_LE(world.GetBQ(pt, curPlayer), BQ_HUT);
    }
    auto ai = AIFactory::Create(AI::Info(AI::DEFAULT, AI::HARD), curPlayer, world);
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
        if(containsBldType(bldSites, BLD_BARRACKS) || containsBldType(bldSites, BLD_GUARDHOUSE))
            break;
    }
    BOOST_REQUIRE(containsBldType(bldSites, BLD_BARRACKS) || containsBldType(bldSites, BLD_GUARDHOUSE));
}

BOOST_AUTO_TEST_SUITE_END()
