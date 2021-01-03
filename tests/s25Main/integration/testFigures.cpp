// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "GamePlayer.h"
#include "PointOutput.h"
#include "buildings/nobBaseWarehouse.h"
#include "factories/BuildingFactory.h"
#include "figures/noFigure.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "nodeObjs/noFlag.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(FigureTests)

BOOST_FIXTURE_TEST_CASE(DestroyWHWithFigure, WorldWithGCExecution2P)
{
    MapPoint flagPos = world.GetNeighbour(hqPos, Direction::SouthEast);
    MapPoint whPos(flagPos.x + 5, flagPos.y);
    auto* wh = static_cast<nobBaseWarehouse*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Storehouse, whPos, curPlayer, Nation::Romans));
    Inventory inv;
    inv.Add(Job::Helper, 1);
    wh->AddGoods(inv, true);
    const unsigned numHelpers = world.GetPlayer(curPlayer).GetInventory().people[Job::Helper]; //-V807
    MapPoint whFlagPos = world.GetNeighbour(whPos, Direction::SouthEast);
    // Build a road -> Requests a worker
    this->BuildRoad(whFlagPos, false, std::vector<Direction>(2, Direction::West));
    BOOST_REQUIRE_EQUAL(wh->GetNumRealFigures(Job::Helper), 0u);
    BOOST_REQUIRE_EQUAL(wh->GetLeavingFigures().size(), 1u);
    // Destroy Road
    this->DestroyFlag(whFlagPos - MapPoint(2, 0));
    BOOST_REQUIRE_EQUAL(wh->GetNumRealFigures(Job::Helper), 1u);
    BOOST_REQUIRE_EQUAL(wh->GetLeavingFigures().size(), 0u);

    this->BuildRoad(whFlagPos, false, std::vector<Direction>(2, Direction::West));
    const noFigure* fig = wh->GetLeavingFigures().front();
    // Destroy wh -> Worker released
    this->DestroyFlag(whFlagPos);
    BOOST_REQUIRE_EQUAL(world.GetPlayer(curPlayer).GetInventory().people[Job::Helper], numHelpers);
    BOOST_REQUIRE_EQUAL(fig->GetPos(), whPos);
    BOOST_REQUIRE(fig->IsWandering());

    // Same for HQ
    // Build a road -> Requests a worker
    this->BuildRoad(flagPos, false, std::vector<Direction>(2, Direction::West));
    wh = world.GetSpecObj<nobBaseWarehouse>(hqPos);
    BOOST_REQUIRE_EQUAL(wh->GetLeavingFigures().size(), 1u);
    fig = wh->GetLeavingFigures().front();
    // Destroy wh -> Worker released
    this->DestroyFlag(flagPos);
    BOOST_REQUIRE_EQUAL(world.GetPlayer(curPlayer).GetInventory().people[Job::Helper], numHelpers);
    BOOST_REQUIRE_EQUAL(fig->GetPos(), hqPos);
    BOOST_REQUIRE(fig->IsWandering());
}

BOOST_FIXTURE_TEST_CASE(DestroyWHWithWare, WorldWithGCExecution2P)
{
    MapPoint flagPos = world.GetNeighbour(hqPos, Direction::SouthEast);
    MapPoint whFlagPos(flagPos.x + 5, flagPos.y);
    MapPoint whPos = world.GetNeighbour(whFlagPos, Direction::NorthWest);
    BuildingFactory::CreateBuilding(world, BuildingType::HarborBuilding, whPos, curPlayer, Nation::Romans);
    // Build a road
    this->BuildRoad(whFlagPos, false, std::vector<Direction>(5, Direction::West));
    // Request people and wares
    this->SetInventorySetting(whPos, GoodType::Wood, EInventorySetting::Collect);
    this->SetInventorySetting(whPos, Job::Woodcutter, EInventorySetting::Collect);
    auto* flag = world.GetSpecObj<noFlag>(flagPos);
    RTTR_EXEC_TILL(200, flag->GetNumWares() > 0);
    // Destroy wh -> Cancel wares and figures
    this->DestroyFlag(whFlagPos);
}

BOOST_AUTO_TEST_SUITE_END()
