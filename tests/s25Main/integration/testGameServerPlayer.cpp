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

#include "network/GameProtocol.h"
#include "network/GameServerPlayer.h"
#include "s25util/Socket.h"
#include <rttr/test/MockClock.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(GameServerPlayerSuite)

BOOST_FIXTURE_TEST_CASE(Ping, rttr::test::MockClockFixture)
{
    using namespace std::chrono;
    Socket sock;
    GameServerPlayer player(1, sock);
    currentTime += seconds(5);
    player.doPing();
    // Not connected
    BOOST_TEST_REQUIRE(player.sendQueue.empty());
    player.setActive();
    player.doPing();
    // Cmd sent
    BOOST_TEST_REQUIRE(!player.sendQueue.empty());
    player.sendQueue.pop();
    currentTime += milliseconds(15);
    BOOST_TEST_REQUIRE(player.calcPingTime() == 15u);
    currentTime += milliseconds(seconds(PING_RATE)) / 2;
    player.doPing();
    // To fast
    BOOST_TEST_REQUIRE(player.sendQueue.empty());
    currentTime += milliseconds(seconds(PING_RATE)) / 2;
    player.doPing();
    // Cmd sent
    BOOST_TEST_REQUIRE(!player.sendQueue.empty());
    player.sendQueue.pop();
    currentTime += seconds(PING_RATE) * 2;
    player.doPing();
    // Already pinging
    BOOST_TEST_REQUIRE(player.sendQueue.empty());
    BOOST_TEST_REQUIRE(player.calcPingTime() == (2000u + 15u) / 2u); // Smoothed value
}

BOOST_AUTO_TEST_SUITE_END()
