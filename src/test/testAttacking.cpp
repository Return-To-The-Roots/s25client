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
#include "test/WorldWithGCExecution.h"
#include "factories/BuildingFactory.h"
#include "pathfinding/FindPathForRoad.h"
#include "buildings/nobMilitary.h"
#include "world/GameWorldViewer.h"
#include "gameData/SettingTypeConv.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(AttackSuite)

// Size is choosen based on current maximum attacking distances!
struct AttackFixture: public WorldWithGCExecution<3, 58, 38>
{
    typedef WorldWithGCExecution<3, 58, 38> Parent;
    using Parent::world;
    using Parent::curPlayer;

    /// Positions of the players HQ
    std::array<MapPoint, 3> hqPos;
    /// Military buildings of players 1 and 2 (for 1, one is close and one far away from player 1)
    const nobMilitary *milBld1Near, *milBld1Far, *milBld2;
    GameWorldViewer gwv;

    AttackFixture(): gwv(curPlayer, world)
    {
        for(unsigned i = 0; i < 3; i++)
            hqPos[i] = world.GetPlayer(i).GetHQPos();
        // Assert player positions: 0: Top-Left, 1: Top-Right, 2: Bottom-Left
        BOOST_REQUIRE_LT(hqPos[0].x, hqPos[1].x);
        BOOST_REQUIRE_LT(hqPos[0].y, hqPos[2].y);
        BOOST_REQUIRE_LT(hqPos[1].y, hqPos[2].y);
        MapPoint bldPos = hqPos[1] - MapPoint(8, 0);
        BOOST_REQUIRE_EQUAL(world.GetBQ(bldPos, 1), BQ_CASTLE);
        milBld1Near = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(&world, BLD_WATCHTOWER, bldPos, 1, NAT_ROMANS));
        BOOST_REQUIRE(milBld1Near);
        bldPos = hqPos[1] + MapPoint(5, 4);
        BOOST_REQUIRE_EQUAL(world.GetBQ(bldPos, 1), BQ_CASTLE);
        milBld1Far = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(&world, BLD_WATCHTOWER, bldPos, 1, NAT_ROMANS));
        BOOST_REQUIRE(milBld1Far);
        bldPos = hqPos[2] - MapPoint(0, 6);
        BOOST_REQUIRE_EQUAL(world.GetBQ(bldPos, 2), BQ_CASTLE);
        milBld2 = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(&world, BLD_WATCHTOWER, bldPos, 2, NAT_BABYLONIANS));
        BOOST_REQUIRE(milBld2);
        curPlayer = 1;
        BuildRoadForBlds(hqPos[1], milBld1Near->GetPos());
        BuildRoadForBlds(hqPos[1], milBld1Far->GetPos());
        curPlayer = 2;
        BuildRoadForBlds(hqPos[2], milBld2->GetPos());
        for(unsigned i=0; i<3; i++)
        {
            curPlayer = i;
            this->ChangeMilitary(MILITARY_SETTINGS_SCALE);
        }
        // Let soldiers get into blds. (6 soldiers, 6 fields distance, 20GFs per field, 30GFs for leaving HQ)
        unsigned numGFs = 6 * (6 * 20 + 30);
        // And 8 fields distance for far bld
        numGFs += 6 * (8 * 20 + 30);
        for(unsigned gf = 0; gf < numGFs; gf++)
            this->em.ExecuteNextGF();
        BOOST_REQUIRE_EQUAL(milBld1Near->GetTroopsCount(), 6u);
        BOOST_REQUIRE_EQUAL(milBld1Far->GetTroopsCount(), 6u);
        BOOST_REQUIRE_EQUAL(milBld2->GetTroopsCount(), 6u);
    }

    /// Constructs a road connecting 2 buildings and checks for success
    void BuildRoadForBlds(const MapPoint bldPosFrom, const MapPoint bldPosTo)
    {
        const MapPoint start = world.GetNeighbour(bldPosFrom, Direction::SOUTHEAST);
        const MapPoint end = world.GetNeighbour(bldPosTo, Direction::SOUTHEAST);
        std::vector<unsigned char> road = FindPathForRoad(world, start, end, world, false);
        BOOST_REQUIRE(!road.empty());
        this->BuildRoad(start, false, road);
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(start, road.front()), RoadSegment::RT_NORMAL + 1);
    }

    void SetCurPlayer(unsigned playerIdx)
    {
        curPlayer = playerIdx;
        gwv.ChangePlayer(playerIdx);
    }
};

BOOST_FIXTURE_TEST_CASE(NumSoldiersForAttack, AttackFixture)
{
    // Player 0 has no military blds -> Can't attack
    SetCurPlayer(0);
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(hqPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(hqPos[2]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(milBld1Near->GetPos()), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(milBld1Far->GetPos()), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(milBld2->GetPos()), 0u);
    SetCurPlayer(1);
    // No self attack
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(hqPos[1]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(milBld1Near->GetPos()), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(milBld1Far->GetPos()), 0u);
    // Attack both others
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(hqPos[0]), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(hqPos[2]), 5u);
    // This is in the extended range of the far bld -> 2 more (with current range)
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(milBld2->GetPos()), 7u);
    SetCurPlayer(2);
    // No self attack
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(hqPos[2]), 0u);
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(milBld2->GetPos()), 0u);
    // Attack both others
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(hqPos[0]), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(hqPos[1]), 5u);
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(milBld1Near->GetPos()), 5u);
    // Counterpart: 2 possible for far bld
    BOOST_REQUIRE_EQUAL(gwv.GetAvailableSoldiersForAttack(milBld1Far->GetPos()), 2u);
}

BOOST_AUTO_TEST_SUITE_END()
