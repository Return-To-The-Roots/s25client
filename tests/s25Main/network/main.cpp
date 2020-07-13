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

#define BOOST_TEST_MODULE RTTR_Network

#include "TestServer.h"
#include <rttr/test/Fixture.hpp>
#include <s25util/Socket.h>
#include <boost/test/unit_test.hpp>

//#include <vld.h>

struct NetworkFixture : rttr::test::Fixture
{
    NetworkFixture() { Socket::Initialize(); }
    ~NetworkFixture() { Socket::Shutdown(); }
};

BOOST_GLOBAL_FIXTURE(NetworkFixture);

BOOST_AUTO_TEST_CASE(TestServer_Works)
{
    TestServer server;
    // Run might need to be called multiple times as connection and drop might not be immediate
    auto runServer = [&server]() {
        const auto result = server.run();
        for(int i = 0; i < 5; i++)
        {
            BOOST_TEST(server.run() == result);
        }
        return result;
    };
    BOOST_TEST(!runServer());
    BOOST_TEST_REQUIRE(server.listen(1337));
    BOOST_TEST(runServer());

    Socket sock;
    BOOST_TEST_REQUIRE(sock.Connect("localhost", 1337, false));
    BOOST_TEST(server.run(true));
    BOOST_TEST_REQUIRE(server.connections.size() == 1u);
    BOOST_TEST(server.stop());
    BOOST_TEST_REQUIRE(server.connections.empty());

    BOOST_TEST_REQUIRE(server.listen(1337));
    Socket sock2;
    BOOST_TEST_REQUIRE(sock.Connect("localhost", 1337, false));
    BOOST_TEST(server.run(true));
    BOOST_TEST_REQUIRE(sock2.Connect("localhost", 1337, false));
    BOOST_TEST(server.run(true));
    BOOST_TEST_REQUIRE(server.connections.size() == 2u);
    BOOST_TEST(runServer());
    sock.Close();
    BOOST_TEST(runServer());
    BOOST_TEST_REQUIRE(server.connections.size() == 1u);
}
