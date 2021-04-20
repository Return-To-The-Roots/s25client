// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
    BOOST_TEST_REQUIRE(!world.GetPlayer(0).IsDefeated());
    BOOST_TEST_REQUIRE(!world.GetPlayer(1).IsDefeated());
    // Destroy HQ -> defeated
    world.DestroyNO(world.GetPlayer(1).GetHQPos()); //-V522
    BOOST_TEST_REQUIRE(!world.GetPlayer(0).IsDefeated());
    BOOST_TEST_REQUIRE(world.GetPlayer(1).IsDefeated());
    // Destroy HQ but leave a military bld
    MapPoint milBldPos = world.MakeMapPoint(world.GetPlayer(0).GetFirstWH()->GetPos() + Position(4, 0)); //-V522
    auto* milBld = dynamic_cast<nobMilitary*>(
      BuildingFactory::CreateBuilding(world, BuildingType::Watchtower, milBldPos, 0, Nation::Babylonians));
    auto& sld = world.AddFigure(milBldPos, std::make_unique<nofPassiveSoldier>(milBldPos, 0, milBld, milBld, 0));
    milBld->GotWorker(Job::Private, sld);
    sld.WalkToGoal();
    world.DestroyNO(world.GetPlayer(0).GetHQPos());
    BOOST_TEST_REQUIRE(!world.GetPlayer(0).IsDefeated());
    // Destroy this -> defeated
    world.DestroyNO(milBldPos);
    BOOST_TEST_REQUIRE(world.GetPlayer(0).IsDefeated());
}
