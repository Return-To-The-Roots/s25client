// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "WorldWithGCExecution.h"
#include "ai/AIPlayer.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "factories/AIFactory.h"
#include "nodeObjs/noTree.h"
#include "gameData/BuildingProperties.h"
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

struct IsBldType
{
    BuildingType type;
    IsBldType(BuildingType type) : type(type) {}
    bool operator()(const noBaseBuilding* bld) { return bld->GetBuildingType() == type; }
};

template<class T_Col>
inline bool containsBldType(const T_Col& collection, BuildingType type)
{
    return std::find_if(collection.begin(), collection.end(), IsBldType(type)) != collection.end();
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

BOOST_FIXTURE_TEST_CASE(BuildWoodIndustry, WorldWithGCExecution<1>)
{
    // Place a few trees
    BOOST_FOREACH(const MapPoint& pt, world.GetPointsInRadius(hqPos + MapPoint(4, 0), 2))
    {
        if(!world.GetNode(pt).obj)
            world.SetNO(pt, new noTree(pt, 0, 3));
    }
    const GamePlayer& player = world.GetPlayer(curPlayer);
    AIPlayer* ai = AIFactory::Create(AI::Info(AI::DEFAULT, AI::HARD), curPlayer, world);
    // Build a woodcutter, sawmill and forester at some point
    for(unsigned gf = 0; gf < 2000;)
    {
        std::vector<gc::GameCommandPtr> aiGcs = ai->GetGameCommands();
        ai->FetchGameCommands();
        for(unsigned i = 0; i < 5; i++, gf++)
        {
            em.ExecuteNextGF();
            ai->RunGF(em.GetCurrentGF(), i == 0);
        }
        BOOST_FOREACH(gc::GameCommandPtr& gc, aiGcs)
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

BOOST_FIXTURE_TEST_CASE(ExpandWhenNoSpace, WorldWithGCExecution<1>)
{
    const GamePlayer& player = world.GetPlayer(curPlayer);
    // No space for saw mill
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        if(world.GetBQ(pt, curPlayer) > BQ_HUT)
            world.GetNodeWriteable(pt).bq = BQ_HUT;
    }
    AIPlayer* ai = AIFactory::Create(AI::Info(AI::DEFAULT, AI::HARD), curPlayer, world);
    const std::list<noBuildingSite*>& bldSites = player.GetBuildingRegister().GetBuildingSites();
    // Can't build sawmill -> Expand anyway
    for(unsigned gf = 0; gf < 2000;)
    {
        std::vector<gc::GameCommandPtr> aiGcs = ai->GetGameCommands();
        ai->FetchGameCommands();
        for(unsigned i = 0; i < 5; i++, gf++)
        {
            em.ExecuteNextGF();
            ai->RunGF(em.GetCurrentGF(), i == 0);
        }
        BOOST_FOREACH(gc::GameCommandPtr& gc, aiGcs)
        {
            gc->Execute(world, curPlayer);
        }
        if(containsBldType(bldSites, BLD_BARRACKS) || containsBldType(bldSites, BLD_GUARDHOUSE))
            break;
    }
    BOOST_REQUIRE(containsBldType(bldSites, BLD_BARRACKS) || containsBldType(bldSites, BLD_GUARDHOUSE));
}

BOOST_AUTO_TEST_SUITE_END()
