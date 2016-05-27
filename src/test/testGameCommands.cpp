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
#include "factories/GameCommandFactory.h"
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
class WorldWithGCExecution1P: public WorldWithGCExecution<1>
{
public:
    using WorldWithGCExecution<1>::world;
    using WorldWithGCExecution<1>::curPlayer;
    using WorldWithGCExecution<1>::hqPos;
};

BOOST_FIXTURE_TEST_CASE(PlaceFlagTest, WorldWithGCExecution1P)
{
    MapPoint flagPt = this->hqPos + MapPoint(3, 0);
    this->SetFlag(flagPt);
    BOOST_REQUIRE_EQUAL(world.GetNO(flagPt)->GetType(), NOP_FLAG);
    BOOST_REQUIRE(world.GetSpecObj<noRoadNode>(flagPt));
    BOOST_REQUIRE_EQUAL(world.GetSpecObj<noRoadNode>(flagPt)->GetPos(), flagPt);
    BOOST_REQUIRE_EQUAL(world.GetSpecObj<noRoadNode>(flagPt)->GetPlayer(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
