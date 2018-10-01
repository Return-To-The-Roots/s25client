// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "factories/BuildingFactory.h"
#include "gameData/BuildingProperties.h"
#include "test/CreateEmptyWorld.h"
#include "test/WorldFixture.h"
#include <boost/test/unit_test.hpp>

typedef WorldFixture<CreateEmptyWorld, 1> EmptyWorldFixture1P;

BOOST_FIXTURE_TEST_CASE(IsType, EmptyWorldFixture1P)
{
    const MapPoint bldPos(0, 0);
    for(unsigned i = 0; i < NUM_BUILDING_TYPES; i++)
    {
        BuildingType bldType = BuildingType(i);
        if(!BuildingProperties::IsValid(bldType))
            continue;
        noBuilding* bld = BuildingFactory::CreateBuilding(world, bldType, bldPos, 0, NAT_ROMANS);
        BOOST_REQUIRE(bld);
        if(BuildingProperties::IsMilitary(bldType))
            BOOST_REQUIRE(dynamic_cast<nobMilitary*>(bld));
        else
            BOOST_REQUIRE(!dynamic_cast<nobMilitary*>(bld));
        if(BuildingProperties::IsWareHouse(bldType))
            BOOST_REQUIRE(dynamic_cast<nobBaseWarehouse*>(bld));
        else
            BOOST_REQUIRE(!dynamic_cast<nobBaseWarehouse*>(bld));
        if(BuildingProperties::IsMine(bldType))
            BOOST_REQUIRE(dynamic_cast<nobUsual*>(bld)); // At least a usual
        world.DestroyNO(bldPos);
        // Destroy fire
        world.DestroyNO(bldPos);
    }
}
