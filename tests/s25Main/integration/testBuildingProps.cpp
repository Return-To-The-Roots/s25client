// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "factories/BuildingFactory.h"
#include "helpers/EnumRange.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "gameData/BuildingProperties.h"
#include <boost/test/unit_test.hpp>

namespace {
using EmptyWorldFixture1P = WorldFixture<CreateEmptyWorld, 1>;
}

BOOST_FIXTURE_TEST_CASE(IsType, EmptyWorldFixture1P)
{
    const MapPoint bldPos(0, 0);
    for(const auto bldType : helpers::enumRange<BuildingType>())
    {
        if(!BuildingProperties::IsValid(bldType))
            continue;
        noBuilding* bld = BuildingFactory::CreateBuilding(world, bldType, bldPos, 0, Nation::Romans);
        BOOST_TEST_REQUIRE(bld);
        if(BuildingProperties::IsMilitary(bldType))
            BOOST_TEST_REQUIRE(dynamic_cast<nobMilitary*>(bld));
        else
            BOOST_TEST_REQUIRE(!dynamic_cast<nobMilitary*>(bld));
        if(BuildingProperties::IsWareHouse(bldType))
            BOOST_TEST_REQUIRE(dynamic_cast<nobBaseWarehouse*>(bld));
        else
            BOOST_TEST_REQUIRE(!dynamic_cast<nobBaseWarehouse*>(bld));
        if(BuildingProperties::IsMine(bldType))
            BOOST_TEST_REQUIRE(dynamic_cast<nobUsual*>(bld)); // At least a usual
        if(BuildingProperties::IsUsual(bldType))
            BOOST_TEST_REQUIRE(dynamic_cast<nobUsual*>(bld));
        world.DestroyNO(bldPos);
        // Destroy fire
        world.DestroyNO(bldPos);
    }
}
