// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
        GamePlayer& player = world.getPlayer(0);
        hq = world.getSpecObj<nobBaseWarehouse>(player.getHQPos());
        hb = static_cast<nobHarborBuilding*>(BuildingFactory::createBuilding(
          world, BuildingType::HarborBuilding, player.getHQPos() + MapPoint(4, 0), 0, Nation::Romans));
        world.BuildRoad(0, false, hq->getFlagPos(), {4, Direction::East});
    }

    nobBaseWarehouse* hq;
    nobHarborBuilding* hb;
};
} // namespace

BOOST_FIXTURE_TEST_CASE(StartExpeditionThenCancel, HarborFixture)
{
    addStartResources();
    BOOST_TEST_REQUIRE(hq->getNumRealFigures(Job::Builder) >= 1u); // Must have a builder
    const auto numLeavingFigs = hq->getLeavingFigures().size();
    hb->startExpedition();
    // Builder should be ordered
    BOOST_TEST_REQUIRE(hq->getLeavingFigures().size() == numLeavingFigs + 1u);
    hb->stopExpedition();
    // Builder should still come
    BOOST_TEST_REQUIRE(hq->getLeavingFigures().size() == numLeavingFigs + 1u);
}

BOOST_FIXTURE_TEST_CASE(StartExpeditionThenDestroy, HarborFixture)
{
    addStartResources();
    BOOST_TEST_REQUIRE(hq->getNumRealFigures(Job::Builder) >= 1u); // Must have a builder
    const auto numLeavingFigs = hq->getLeavingFigures().size();
    hb->startExpedition();
    // Builder should be ordered
    BOOST_TEST_REQUIRE(hq->getLeavingFigures().size() == numLeavingFigs + 1u);
    world.DestroyBuilding(hb->getPos(), 0);
    // Builder should be canceled
    BOOST_TEST_REQUIRE(hq->getLeavingFigures().size() == numLeavingFigs);
}
