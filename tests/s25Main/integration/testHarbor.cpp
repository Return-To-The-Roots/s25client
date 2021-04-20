// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
