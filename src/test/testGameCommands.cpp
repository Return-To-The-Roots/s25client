// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "EmptyWorldFixture.h"
#include "GamePlayer.h"
#include "nodeObjs/noBase.h"
#include "nodeObjs/noEnvObject.h"
#include "factories/GameCommandFactory.h"
#include "gameTypes/VisualSettings.h"
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>

#define RTTR_FOREACH_PT(TYPE, WIDTH, HEIGHT)    \
    for(TYPE pt(0, 0); pt.y < (HEIGHT); ++pt.y) \
        for(pt.x = 0; pt.x < (WIDTH); ++pt.x)

template<typename T>
std::ostream& operator<<(std::ostream &out, const Point<T>& point)
{
    return out << "(" << point.x << ", " << point.y << ")";
}

BOOST_AUTO_TEST_SUITE(GameCommandSuite)

template<unsigned T_numPlayers>
class WorldWithGCExecution: public EmptyWorldFixture<T_numPlayers>, public GameCommandFactory
{
public:
    using EmptyWorldFixture<T_numPlayers>::world;

    unsigned curPlayer;
    MapPoint hqPos;
    WorldWithGCExecution(): curPlayer(0), hqPos(world.GetPlayer(curPlayer).GetHQPos()){}
protected:
    virtual bool AddGC(gc::GameCommand* gc) override
    {
        gc->Execute(world, curPlayer);
        deletePtr(gc);
        return true;
    }
};

// Avoid having to use "this->" to access those
class WorldWithGCExecution2P: public WorldWithGCExecution<2>
{
public:
    using WorldWithGCExecution<2>::world;
    using WorldWithGCExecution<2>::curPlayer;
    using WorldWithGCExecution<2>::hqPos;
};

BOOST_FIXTURE_TEST_CASE(PlaceFlagTest, WorldWithGCExecution2P)
{
    MapPoint flagPt = this->hqPos + MapPoint(4, 0);
    // Place flag for other player:
    curPlayer = 1;
    this->SetFlag(flagPt);
    // Wrong terrain
    BOOST_REQUIRE(!world.GetSpecObj<noRoadNode>(flagPt));

    curPlayer = 0;
    this->SetFlag(flagPt);
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt)->GetType(), NOP_FLAG);
    noRoadNode* flag = world.GetSpecObj<noRoadNode>(flagPt);
    BOOST_REQUIRE(flag);
    BOOST_REQUIRE_EQUAL(flag->GetPos(), flagPt);
    BOOST_REQUIRE_EQUAL(flag->GetPlayer(), 0);
    // Flag blocked
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt, Direction::WEST).bq, BQ_NOTHING);
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt, Direction::NORTHEAST).bq, BQ_NOTHING);
    // This flag = house flag
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt, Direction::NORTHWEST).bq, BQ_CASTLE);
    // Flag blocks castle
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt, Direction::EAST).bq, BQ_HOUSE);
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt, Direction::SOUTHEAST).bq, BQ_HOUSE);
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt, Direction::SOUTHWEST).bq, BQ_HOUSE);
    // Place flag again
    this->SetFlag(flagPt);
    // Nothing should be changed
    BOOST_REQUIRE_EQUAL(world.GetSpecObj<noRoadNode>(flagPt), flag);

    // Place flag at neighbour
    for(int dir = 0; dir < 6; dir++)
    {
        MapPoint curPt = world.GetNeighbour(flagPt, dir);
        this->SetFlag(curPt);
        // Should not work
        BOOST_REQUIRE(!world.GetSpecObj<noRoadNode>(curPt));
    }

    unsigned objCt = GameObject::GetObjCount();
    this->DestroyFlag(flagPt);
    // Removed from map
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt)->GetType(), NOP_NOTHING);
    // Removed from game
    BOOST_REQUIRE_EQUAL(GameObject::GetObjCount(), objCt - 1);
    // And everything clear now
    for(int dir = 0; dir < 6; dir++)
        BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt, dir).bq, BQ_CASTLE);
}

BOOST_FIXTURE_TEST_CASE(RoadTest, WorldWithGCExecution2P)
{
    MapPoint flagPt = this->hqPos + MapPoint(4, 0);
    // 2 flags outside range of HQ
    this->SetFlag(flagPt);
    this->SetFlag(flagPt + MapPoint(4, 0));
    // Build road with 3 segments:
    // a1) invalid start pt -> No road
    this->BuildRoad(flagPt + MapPoint(2, 0), false, std::vector<unsigned char>(4, 3));
    for(unsigned i = 0; i < 6; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), 3), 0);
    // a2) invalid player
    curPlayer = 1;
    this->BuildRoad(flagPt, false, std::vector<unsigned char>(4, 3));
    for(unsigned i = 0; i < 6; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), 3), 0);
    curPlayer = 0;

    // b) Flag->Flag ->OK
    this->BuildRoad(flagPt, false, std::vector<unsigned char>(4, 3));
    for(unsigned i = 0; i < 4; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), 3), 1);
    // End of road
    BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(4, 0), 3), 0);
    // BQ on road
    BOOST_REQUIRE_EQUAL(world.GetNode(flagPt + MapPoint(1, 0)).bq, BQ_NOTHING);
    BOOST_REQUIRE_EQUAL(world.GetNode(flagPt + MapPoint(2, 0)).bq, BQ_FLAG);
    BOOST_REQUIRE_EQUAL(world.GetNode(flagPt + MapPoint(3, 0)).bq, BQ_NOTHING);
    BOOST_REQUIRE_EQUAL(world.GetNode(flagPt + MapPoint(4, 0)).bq, BQ_NOTHING);
    // BQ above road
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::NORTHWEST).bq, BQ_CASTLE);  // Flag could be build
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::NORTHEAST).bq, BQ_FLAG);    // only flag possible
    // BQ below road (Castle blocked by road)
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::SOUTHEAST).bq, BQ_HOUSE);
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::SOUTHWEST).bq, BQ_HOUSE);

    // Set another flag on the road
    // c) to close
    this->SetFlag(flagPt + MapPoint(1, 0));
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt + MapPoint(1, 0))->GetType(), NOP_NOTHING);
    // d) middle -> ok
    this->SetFlag(flagPt + MapPoint(2, 0));
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt + MapPoint(2, 0))->GetType(), NOP_FLAG);
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::NORTHWEST).bq, BQ_CASTLE);  // Flag could be build
    BOOST_REQUIRE_EQUAL(world.GetNeighbourNode(flagPt + MapPoint(2, 0), Direction::NORTHEAST).bq, BQ_NOTHING);    // no more flag possible
    // e) upgrade
    this->UpgradeRoad(flagPt, 3);
    for(unsigned i = 0; i < 2; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), 3), 2);
    for(unsigned i = 2; i < 4; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), 3), 1);
    // f) destroy middle flag -> Road destroyed
    this->DestroyFlag(flagPt + MapPoint(2, 0));
    for(unsigned i = 0; i < 4; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), 3), 0);

    // g) Road with no existing end flag -> Road and flag place
    this->BuildRoad(flagPt, false, std::vector<unsigned char>(2, 3));
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt + MapPoint(2, 0))->GetType(), NOP_FLAG);
    for(unsigned i = 0; i < 2; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), 3), 1);

    // h) Non-blocking env. object
    world.SetNO(flagPt + MapPoint(3, 0), new noEnvObject(flagPt, 512));
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt + MapPoint(3, 0))->GetType(), NOP_ENVIRONMENT);
    this->BuildRoad(flagPt + MapPoint(2, 0), false, std::vector<unsigned char>(2, 3));
    for(unsigned i = 2; i < 4; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), 3), 1);
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt + MapPoint(3, 0))->GetType(), NOP_NOTHING);

    // Remove other flags
    this->DestroyFlag(flagPt + MapPoint(2, 0));
    this->DestroyFlag(flagPt + MapPoint(4, 0));

    // i) outside player territory
    // i1) border
    this->BuildRoad(flagPt, false, std::vector<unsigned char>(HQ_RADIUS - (flagPt.x - hqPos.x), 3));
    for(unsigned i = 0; i <= HQ_RADIUS; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), 3), 0);
    // i2) territory
    this->BuildRoad(flagPt, false, std::vector<unsigned char>(HQ_RADIUS - (flagPt.x - hqPos.x) + 1, 3));
    for(unsigned i = 0; i <= HQ_RADIUS; i++)
        BOOST_REQUIRE_EQUAL(world.GetPointRoad(flagPt + MapPoint(i, 0), 3), 0);
}

BOOST_FIXTURE_TEST_CASE(DistributionAndBuildOrderTest, WorldWithGCExecution2P)
{
    // Execute for both players to make sure they don't influence each other
    for(; curPlayer < 2; curPlayer++)
    {
        Distributions inDist;
        for(unsigned i = 0; i < inDist.size(); i++)
            inDist[i] = rand();
        bool orderType = rand() % 2 == 0;
        BuildOrders inBuildOrder;
        for(unsigned i = 0; i < inBuildOrder.size(); i++)
            inBuildOrder[i] = rand();
        this->ChangeDistribution(inDist);
        this->ChangeBuildOrder(orderType, inBuildOrder);
        // TODO: Use better getters once available
        VisualSettings outSettings;
        world.GetPlayer(curPlayer).FillVisualSettings(outSettings);
        for(unsigned i = 0; i < inDist.size(); i++)
            BOOST_CHECK_EQUAL(outSettings.distribution[i], inDist[i]);
        BOOST_REQUIRE_EQUAL(outSettings.useCustomBuildOrder, orderType);
        for(unsigned i = 0; i < inBuildOrder.size(); i++)
            BOOST_CHECK_EQUAL(outSettings.build_order[i], inBuildOrder[i]);
    }
};

BOOST_AUTO_TEST_SUITE_END()
