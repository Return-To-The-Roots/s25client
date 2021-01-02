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

#include "GamePlayer.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "factories/BuildingFactory.h"
#include "figures/nofPassiveSoldier.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include <boost/test/unit_test.hpp>

using WorldFixtureEmpty2P = WorldFixture<CreateEmptyWorld, 2>;

BOOST_FIXTURE_TEST_CASE(Defeat, WorldFixtureEmpty2P)
{
    BOOST_REQUIRE(!world.GetPlayer(0).IsDefeated());
    BOOST_REQUIRE(!world.GetPlayer(1).IsDefeated());
    // Destroy HQ -> defeated
    world.DestroyNO(world.GetPlayer(1).GetHQPos()); //-V522
    BOOST_REQUIRE(!world.GetPlayer(0).IsDefeated());
    BOOST_REQUIRE(world.GetPlayer(1).IsDefeated());
    // Destroy HQ but leave a military bld
    MapPoint milBldPos = world.MakeMapPoint(world.GetPlayer(0).GetFirstWH()->GetPos() + Position(4, 0)); //-V522
    auto* milBld = dynamic_cast<nobMilitary*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, milBldPos, 0, NAT_BABYLONIANS));
    auto* sld = new nofPassiveSoldier(milBldPos, 0, milBld, milBld, 0);
    world.AddFigure(milBldPos, sld);
    milBld->GotWorker(Job::Private, sld);
    sld->WalkToGoal();
    world.DestroyNO(world.GetPlayer(0).GetHQPos());
    BOOST_REQUIRE(!world.GetPlayer(0).IsDefeated());
    // Destroy this -> defeated
    world.DestroyNO(milBldPos);
    BOOST_REQUIRE(world.GetPlayer(0).IsDefeated());
}
