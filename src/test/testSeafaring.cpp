// Copyright (c) 2016 Settlers Freaks (sf-team at siedler25.org)
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
#include "SeaWorldWithGCExecution.h"
#include "GamePlayer.h"
#include "pathfinding/FreePathFinderImpl.h"
#include <boost/test/unit_test.hpp>
#include "factories/BuildingFactory.h"
#include "buildings/nobHarborBuilding.h"

namespace{
    struct PathConditionRoad
    {
        const GameWorldBase& world;

        PathConditionRoad(const GameWorldBase& world): world(world){}

        // Called for every node but the start & goal and should return true, if this point is usable
        FORCE_INLINE bool IsNodeOk(const MapPoint& pt) const
        {
            return world.IsPlayerTerritory(pt) && !world.IsOnRoad(pt) && world.RoadAvailable(false, pt);
        }

        // Called for every edge (node to other node)
        FORCE_INLINE bool IsEdgeOk(const MapPoint&  /*fromPt*/, const unsigned char  /*dir*/) const
        {
            return true;
        }
    };

    std::vector<unsigned char> FindRoadPath(const MapPoint fromPt, const MapPoint toPt, const GameWorldBase& world)
    {
        std::vector<unsigned char> road;
        world.GetFreePathFinder().FindPath(fromPt, toPt, false, 100, &road, NULL, NULL, PathConditionRoad(world));
        return road;
    }

}

BOOST_AUTO_TEST_SUITE(SeafaringTestSuite)

BOOST_FIXTURE_TEST_CASE(HarborPlacing, SeaWorldWithGCExecution)
{
    const GamePlayer& player = world.GetPlayer(curPlayer);
    const MapPoint hqPos = player.GetHQPos();
    const unsigned seaId = 1;
    const unsigned hbId = 1;
    const MapPoint hbPos = world.GetHarborPoint(hbId);
    BOOST_REQUIRE_LT(world.CalcDistance(hqPos, hbPos), HQ_RADIUS);
    
    nobHarborBuilding* harbor = dynamic_cast<nobHarborBuilding*>(BuildingFactory::CreateBuilding(&world, BLD_HARBORBUILDING, hbPos, curPlayer, NAT_ROMANS));
    BOOST_REQUIRE(harbor);
    BOOST_REQUIRE_EQUAL(player.GetHarbors().size(), 1u);
    BOOST_REQUIRE_EQUAL(player.GetHarbors().front(), harbor);
    // A harbor is also a storehouse
    BOOST_REQUIRE_EQUAL(player.GetStorehouses().size(), 2u);
    BOOST_REQUIRE_EQUAL(player.GetHarbors().back(), harbor);
    std::vector<nobHarborBuilding*> harbors;
    BOOST_REQUIRE_EQUAL(world.GetNode(MapPoint(0, 0)).seaId, seaId);
    BOOST_REQUIRE_EQUAL(world.GetSeaId(hbId, Direction::NORTHWEST), seaId);
    player.GetHarborsAtSea(harbors, seaId);
    BOOST_REQUIRE_EQUAL(harbors.size(), 1u);
    BOOST_REQUIRE_EQUAL(harbors.front(), harbor);

    const std::vector<unsigned char> road = FindRoadPath(world.GetNeighbour(hqPos, Direction::SOUTHEAST), world.GetNeighbour(hbPos, Direction::SOUTHEAST), world);
    BOOST_REQUIRE(!road.empty());
}

BOOST_AUTO_TEST_SUITE_END()
