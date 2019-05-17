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

#include "rttrDefines.h" // IWYU pragma: keep
#include "TestServer.h"
#include <libutil/Socket.h>
#include <boost/test/unit_test.hpp>
#include <rttr/test/Fixture.hpp>

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
    BOOST_TEST(!server.run());
    BOOST_TEST(server.listen(1337));
    BOOST_TEST(server.run());

    Socket sock;
    BOOST_TEST(sock.Connect("localhost", 1337, false));
    BOOST_TEST(server.run());
    BOOST_TEST(server.connections.size() == 1u);
    BOOST_TEST(server.stop());
    BOOST_TEST(server.connections.empty());

    BOOST_TEST(server.listen(1337));
    Socket sock2;
    BOOST_TEST(sock.Connect("localhost", 1337, false));
    BOOST_TEST(server.run());
    BOOST_TEST(sock2.Connect("localhost", 1337, false));
    BOOST_TEST(server.run());
    BOOST_TEST_REQUIRE(server.connections.size() == 2u);
    BOOST_TEST(server.run());
    sock.Close();
    BOOST_TEST((server.run() && server.run()));
    BOOST_TEST_REQUIRE(server.connections.size() == 1u);
}
