// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "buildings/nobHQ.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "gameTypes/GameTypesOutput.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(StartWaresSuite)

BOOST_AUTO_TEST_CASE(MinimalLoadouts)
{
    const GoodsAndPeopleCounts minimal = nobHQ::getStartInventory(StartWares::Minimal);
    GoodsAndPeopleCounts expected;
    expected[GoodType::Wood] = 2;
    expected[GoodType::Boards] = 2;
    expected[GoodType::Stones] = 4;
    expected[GoodType::Tongs] = 1;
    expected[GoodType::Hammer] = 1;
    expected[GoodType::Axe] = 1;
    expected[GoodType::Saw] = 1;
    expected[GoodType::Iron] = 2;
    expected[GoodType::IronOre] = 3;
    expected[GoodType::Coal] = 3;
    expected[Job::General] = 2;
    expected[Job::PackDonkey] = 1;
    BOOST_TEST(minimal.goods == expected.goods, boost::test_tools::per_element());
    BOOST_TEST(minimal.people == expected.people, boost::test_tools::per_element());

    const GoodsAndPeopleCounts minimalPlus = nobHQ::getStartInventory(StartWares::MinimalPlus);
    expected[GoodType::Boards] = 10;
    expected[GoodType::Stones] = 10;
    expected[GoodType::Shovel] = 1;
    expected[GoodType::Iron] = 10;
    expected[Job::Private] = 5;
    BOOST_TEST(minimalPlus.goods == expected.goods, boost::test_tools::per_element());
    BOOST_TEST(minimalPlus.people == expected.people, boost::test_tools::per_element());
}

using WorldFixtureEmpty1P = WorldFixture<CreateEmptyWorld, 1>;
BOOST_FIXTURE_TEST_CASE(HQStartsWithMinimalWares, WorldFixtureEmpty1P)
{
    for(const StartWares setting : {StartWares::Minimal, StartWares::MinimalPlus})
    {
        ggs.startWares = setting;
        nobHQ& hq = *world.GetPlayer(0).GetHQ();
        const Inventory before = hq.GetInventory();
        addStartResources();
        const GoodsAndPeopleCounts added = nobHQ::getStartInventory(setting);
        for(const auto good : helpers::enumRange<GoodType>())
            BOOST_TEST(hq.GetInventory()[good] == before[good] + added[good]);
        BOOST_TEST(added[Job::Helper] == 0u);
        BOOST_TEST(hq.GetInventory()[Job::Helper] == before[Job::Helper]);
        BOOST_TEST(hq.GetInventory()[Job::PackDonkey] == before[Job::PackDonkey] + 1u);
    }
}

BOOST_AUTO_TEST_SUITE_END()
