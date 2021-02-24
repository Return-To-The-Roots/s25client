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
#include "RttrForeachPt.h"
#include "buildings/nobBaseWarehouse.h"
#include "factories/BuildingFactory.h"
#include "figures/noFigure.h"
#include "figures/nofGeologist.h"
#include "figures/nofScout_Free.h"
#include "notifications/ResourceNote.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noSign.h"
#include "gameTypes/GameTypesOutput.h"
#include "rttr/test/random.hpp"
#include <boost/test/unit_test.hpp>
#include <vector>

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
    BOOST_TEST_REQUIRE(wh->GetNumRealFigures(Job::Helper) == 0u);
    BOOST_TEST_REQUIRE(wh->GetLeavingFigures().size() == 1u);
    // Destroy Road
    this->DestroyFlag(whFlagPos - MapPoint(2, 0));
    BOOST_TEST_REQUIRE(wh->GetNumRealFigures(Job::Helper) == 1u);
    BOOST_TEST_REQUIRE(wh->GetLeavingFigures().size() == 0u);

    this->BuildRoad(whFlagPos, false, std::vector<Direction>(2, Direction::West));
    const noFigure* fig = wh->GetLeavingFigures().front();
    // Destroy wh -> Worker released
    this->DestroyFlag(whFlagPos);
    BOOST_TEST_REQUIRE(world.GetPlayer(curPlayer).GetInventory().people[Job::Helper] == numHelpers);
    BOOST_TEST_REQUIRE(fig->GetPos() == whPos);
    BOOST_TEST_REQUIRE(fig->IsWandering());

    // Same for HQ
    // Build a road -> Requests a worker
    this->BuildRoad(flagPos, false, std::vector<Direction>(2, Direction::West));
    wh = world.GetSpecObj<nobBaseWarehouse>(hqPos);
    BOOST_TEST_REQUIRE(wh->GetLeavingFigures().size() == 1u);
    fig = wh->GetLeavingFigures().front();
    // Destroy wh -> Worker released
    this->DestroyFlag(flagPos);
    BOOST_TEST_REQUIRE(world.GetPlayer(curPlayer).GetInventory().people[Job::Helper] == numHelpers);
    BOOST_TEST_REQUIRE(fig->GetPos() == hqPos);
    BOOST_TEST_REQUIRE(fig->IsWandering());
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

using EmptyWorldFixture1PBig = WorldFixture<CreateEmptyWorld, 1, 22, 20>;

BOOST_FIXTURE_TEST_CASE(ScoutScouts, EmptyWorldFixture1PBig)
{
    const MapPoint hqFlagPos = world.GetNeighbour(world.GetPlayer(0).GetHQPos(), Direction::SouthEast);
    auto* scout = new nofScout_Free(hqFlagPos, 0, world.GetSpecObj<noRoadNode>(hqFlagPos));
    world.AddFigure(hqFlagPos, scout);
    scout->ActAtFirst();
    const auto countVisibleNodes = [&]() {
        unsigned numVisibleNodes = 0;
        RTTR_FOREACH_PT(MapPoint, world.GetSize())
        {
            if(world.CalcVisiblityWithAllies(pt, 0) != Visibility::Invisible)
                numVisibleNodes++;
        }
        return numVisibleNodes;
    };
    BOOST_TEST_REQUIRE(countVisibleNodes() < prodOfComponents(world.GetSize())); // Must have scoutable nodes
    RTTR_SKIP_GFS(600);
    // All scouted
    BOOST_TEST(countVisibleNodes() == prodOfComponents(world.GetSize()));
}

using EmptyWorldFixture1P = WorldFixture<CreateEmptyWorld, 1>;

BOOST_FIXTURE_TEST_CASE(GeologistPlacesSigns, EmptyWorldFixture1P)
{
    const MapPoint hqFlagPos = world.GetNeighbour(world.GetPlayer(0).GetHQPos(), Direction::SouthEast);
    const MapPoint waterPos = world.GetNeighbour(hqFlagPos, Direction::NorthEast);
    const MapPoint coalPos = world.GetNeighbour(hqFlagPos, Direction::East);
    const MapPoint ironPos = world.GetNeighbour(hqFlagPos, Direction::SouthEast);
    const MapPoint goldPos = world.GetNeighbour(hqFlagPos, Direction::SouthWest);
    const MapPoint granitePos = world.GetNeighbour2(hqFlagPos, 6); // A bit further
    world.GetNodeWriteable(waterPos).resources = Resource(ResourceType::Water, rttr::test::randomValue(1u, 15u));
    world.GetNodeWriteable(coalPos).resources = Resource(ResourceType::Coal, rttr::test::randomValue(1u, 15u));
    world.GetNodeWriteable(ironPos).resources = Resource(ResourceType::Iron, rttr::test::randomValue(1u, 15u));
    world.GetNodeWriteable(goldPos).resources = Resource(ResourceType::Gold, rttr::test::randomValue(1u, 15u));
    world.GetNodeWriteable(granitePos).resources = Resource(ResourceType::Granite, rttr::test::randomValue(1u, 15u));
    // Add some geologists
    for(unsigned i = 0; i < 10; i++)
    {
        auto* geologist = new nofGeologist(hqFlagPos, 0, world.GetSpecObj<noRoadNode>(hqFlagPos));
        world.AddFigure(hqFlagPos, geologist);
        geologist->ActAtFirst();
    }
    std::vector<ResourceNote> notes;
    const auto sub =
      world.GetNotifications().subscribe<ResourceNote>([&notes](const ResourceNote& note) { notes.push_back(note); });

    RTTR_SKIP_GFS(800);
    // All discovered
    BOOST_TEST(notes.size() >= 5u);
    // All from this player
    BOOST_TEST(!helpers::contains_if(notes, [](const ResourceNote& note) { return note.player != 0; }));
    BOOST_TEST(helpers::contains_if(notes, [&](const ResourceNote& note) { return note.pos == waterPos; }));
    BOOST_TEST(helpers::contains_if(notes, [&](const ResourceNote& note) { return note.pos == coalPos; }));
    BOOST_TEST(helpers::contains_if(notes, [&](const ResourceNote& note) { return note.pos == ironPos; }));
    BOOST_TEST(helpers::contains_if(notes, [&](const ResourceNote& note) { return note.pos == goldPos; }));
    BOOST_TEST(helpers::contains_if(notes, [&](const ResourceNote& note) { return note.pos == granitePos; }));
    for(const ResourceNote& note : notes)
    {
        BOOST_TEST(note.res == world.GetNode(note.pos).resources);
        const auto* sign = world.GetSpecObj<noSign>(note.pos);
        BOOST_TEST_REQUIRE(sign);
        BOOST_TEST(sign->GetResource() == note.res);
    }
}

BOOST_AUTO_TEST_SUITE_END()
