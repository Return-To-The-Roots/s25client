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
#include "buildings/nobHarborBuilding.h"
#include "factories/BuildingFactory.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include <boost/test/unit_test.hpp>

namespace {
struct HarborFixture : WorldFixture<CreateEmptyWorld, 1>
{
    HarborFixture()
    {
        GamePlayer& player = world.GetPlayer(0);
        hq = world.GetSpecObj<nobBaseWarehouse>(player.GetHQPos());
        hb = static_cast<nobHarborBuilding*>(BuildingFactory::CreateBuilding(
          world, BuildingType::HarborBuilding, player.GetHQPos() + MapPoint(4, 0), 0, Nation::Romans));
        world.BuildRoad(0, false, hq->GetFlagPos(), {4, Direction::East});
    }

    nobBaseWarehouse* hq;
    nobHarborBuilding* hb;
};
} // namespace

BOOST_FIXTURE_TEST_CASE(StartExpeditionThenCancel, HarborFixture)
{
    BOOST_TEST_REQUIRE(hq->GetNumRealFigures(Job::Builder) >= 1u); // Must have a builder
    const auto numLeavingFigs = hq->GetLeavingFigures().size();
    hb->StartExpedition();
    // Builder should be ordered
    BOOST_TEST_REQUIRE(hq->GetLeavingFigures().size() == numLeavingFigs + 1u);
    hb->StopExpedition();
    // Builder should still come
    BOOST_TEST_REQUIRE(hq->GetLeavingFigures().size() == numLeavingFigs + 1u);
}

BOOST_FIXTURE_TEST_CASE(StartExpeditionThenDestroy, HarborFixture)
{
    BOOST_TEST_REQUIRE(hq->GetNumRealFigures(Job::Builder) >= 1u); // Must have a builder
    const auto numLeavingFigs = hq->GetLeavingFigures().size();
    hb->StartExpedition();
    // Builder should be ordered
    BOOST_TEST_REQUIRE(hq->GetLeavingFigures().size() == numLeavingFigs + 1u);
    world.DestroyBuilding(hb->GetPos(), 0);
    // Builder should be canceled
    BOOST_TEST_REQUIRE(hq->GetLeavingFigures().size() == numLeavingFigs);
}
