// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "GamePlayer.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "factories/BuildingFactory.h"
#include "figures/nofPassiveSoldier.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include <boost/test/unit_test.hpp>

typedef WorldFixture<CreateEmptyWorld, 2> WorldFixtureEmpty2P;

BOOST_FIXTURE_TEST_CASE(Defeat, WorldFixtureEmpty2P)
{
    BOOST_REQUIRE(!world.GetPlayer(0).IsDefeated());
    BOOST_REQUIRE(!world.GetPlayer(1).IsDefeated());
    // Destroy HQ -> defeated
    world.GetPlayer(1).GetFirstWH()->Destroy(); //-V522
    BOOST_REQUIRE(!world.GetPlayer(0).IsDefeated());
    BOOST_REQUIRE(world.GetPlayer(1).IsDefeated());
    // Destroy HQ but leave a military bld
    MapPoint milBldPos = world.MakeMapPoint(world.GetPlayer(0).GetFirstWH()->GetPos() + Position(4, 0)); //-V522
    nobMilitary* milBld = dynamic_cast<nobMilitary*>(BuildingFactory::CreateBuilding(world, BLD_WATCHTOWER, milBldPos, 0, NAT_BABYLONIANS));
    nofPassiveSoldier* sld = new nofPassiveSoldier(milBldPos, 0, milBld, milBld, 0);
    world.AddFigure(milBldPos, sld);
    milBld->GotWorker(JOB_PRIVATE, sld);
    sld->WalkToGoal();
    world.GetPlayer(0).GetFirstWH()->Destroy();
    BOOST_REQUIRE(!world.GetPlayer(0).IsDefeated());
    // Destroy this -> defeated
    milBld->Destroy();
    BOOST_REQUIRE(world.GetPlayer(0).IsDefeated());
}
