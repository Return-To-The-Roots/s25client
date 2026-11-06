// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayer.h"
#include "JoinPlayerInfo.h"
#include "buildings/nobHQ.h"
#include "network/GameMessages.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include "gameTypes/GameTypesOutput.h"
#include "s25util/Serializer.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(StartWaresSuite)

using WorldFixtureEmpty2P = WorldFixture<CreateEmptyWorld, 2>;
BOOST_FIXTURE_TEST_CASE(EachPlayerGetsItsOwnStartWares, WorldFixtureEmpty2P)
{
    ggs.startWares = StartWares::Normal;
    world.GetPlayer(1).startWares = StartWares::VLow;
    addStartResources();

    const GoodsAndPeopleCounts normal = nobHQ::getStartInventory(StartWares::Normal);
    const GoodsAndPeopleCounts vlow = nobHQ::getStartInventory(StartWares::VLow);
    const Inventory& inv0 = world.GetPlayer(0).GetInventory();
    const Inventory& inv1 = world.GetPlayer(1).GetInventory();
    BOOST_TEST(inv0.goods == normal.goods, boost::test_tools::per_element());
    BOOST_TEST(inv0.people == normal.people, boost::test_tools::per_element());
    BOOST_TEST(inv1.goods == vlow.goods, boost::test_tools::per_element());
    BOOST_TEST(inv1.people == vlow.people, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(StartWaresGoOverTheNetwork)
{
    for(const std::optional<StartWares> value :
        {std::optional<StartWares>{}, std::optional<StartWares>{StartWares::VLow},
         std::optional<StartWares>{StartWares::ALot}})
    {
        JoinPlayerInfo player;
        player.ps = PlayerState::Occupied;
        player.name = "Player";
        player.startWares = value;
        Serializer ser;
        player.Serialize(ser);
        const JoinPlayerInfo loaded(ser);
        BOOST_TEST((loaded.startWares == value));
        BOOST_TEST(loaded.name == "Player");

        Serializer msgSer;
        GameMessage_Player_StartWares(3, value).Serialize(msgSer);
        GameMessage_Player_StartWares msg;
        msg.Deserialize(msgSer);
        BOOST_TEST(msg.player == 3u);
        BOOST_TEST((msg.startWares == value));
    }

    // A value past the end is refused
    Serializer bad;
    bad.PushUnsignedChar(helpers::MaxEnumValue_v<StartWares> + 2u);
    BOOST_CHECK_THROW(BasePlayerInfo::popStartWares(bad), std::range_error);
}

BOOST_AUTO_TEST_SUITE_END()
