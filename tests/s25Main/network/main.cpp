// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#define BOOST_TEST_MODULE RTTR_Network

#include "TestServer.h"
#include <rttr/test/Fixture.hpp>
#include <s25util/Socket.h>
#include <boost/test/unit_test.hpp>

#if RTTR_HAS_VLD
#    include <vld.h>
#endif

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
