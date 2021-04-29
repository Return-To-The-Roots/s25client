// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TestServer.h"
#include "liblobby/LobbyClient.h"
#include "liblobby/LobbyInterface.h"
#include "liblobby/LobbyMessages.h"
#include "s25util/md5.hpp"
#include <rttr/test/LogAccessor.hpp>
#include <turtle/mock.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>

struct TestLobbySever : public TestServer, public LobbyMessageInterface
{
    std::string testUser, testPw, testMail;
    boost::ptr_vector<LobbyMessage> messages;
    TestLobbySever() : testUser("testUser"), testPw("testPw"), testMail("Foo@Bar.Foo") {}

    void handleMessages() override
    {
        for(unsigned id = 0; id < connections.size(); ++id)
        {
            Connection& con = connections[id];
            while(!con.recvQueue.empty())
            {
                std::unique_ptr<Message> msg(con.recvQueue.popFront());
                BOOST_TEST(msg->run(this, id));
            }
        }
    }

    Connection acceptConnection(unsigned id, const Socket& so) override
    {
        Connection result(LobbyMessage::create_lobby, so);
        result.sendQueue.push(new LobbyMessage_Id(id));
        return result;
    }

    bool OnNMSLobbyLogin(unsigned id, const unsigned /*revision*/, const std::string& user, const std::string& pass,
                         const std::string& /*version*/) override
    {
        BOOST_TEST_REQUIRE(user == testUser);
        BOOST_TEST_REQUIRE(pass == s25util::md5(testPw).toString());
        connections[id].sendQueue.push(new LobbyMessage_Login_Done(testMail));
        return true;
    }
};

struct LobbyFixture
{
    TestLobbySever lobbyServer;
    uint16_t lobbyPort;
    LobbyFixture() : lobbyPort(5664) { BOOST_TEST_REQUIRE(lobbyServer.listen(lobbyPort)); }
    ~LobbyFixture() { LOBBYCLIENT.Stop(); } // To avoid error msg due to missing server
    void run()
    {
        lobbyServer.run();
        LOBBYCLIENT.Run();
    }
};

namespace {
/* clang-format off */
    MOCK_BASE_CLASS(MockLobbyInterface, LobbyInterface)
    {
    public:
        MockLobbyInterface() { LOBBYCLIENT.AddListener(this); }
        ~MockLobbyInterface() override { LOBBYCLIENT.RemoveListener(this); }
        MOCK_METHOD(LC_Chat, 2)
        MOCK_METHOD(LC_LoggedIn, 1)
        MOCK_METHOD(LC_Connected, 0)
    };
/* clang-format on */
}

BOOST_FIXTURE_TEST_SUITE(Lobby, LobbyFixture)

BOOST_AUTO_TEST_CASE(LobbyConnectAndChat)
{
    rttr::test::LogAccessor logAcc;
    MockLobbyInterface lobby;
    mock::sequence s;
    MOCK_EXPECT(lobby.LC_Connected).once().in(s);
    MOCK_EXPECT(lobby.LC_LoggedIn).once().with(lobbyServer.testMail).in(s);

    BOOST_TEST_REQUIRE(LOBBYCLIENT.Login("localhost", lobbyPort, lobbyServer.testUser, lobbyServer.testPw, false));
    RTTR_REQUIRE_LOG_CONTAINS("Connect", true);
    lobbyServer.run(true);
    for(unsigned i = 0; i < 50; i++)
    {
        run();
        if(LOBBYCLIENT.IsLoggedIn())
            break;
    }
    BOOST_TEST_REQUIRE(LOBBYCLIENT.IsLoggedIn());
    RTTR_REQUIRE_LOG_CONTAINS("NMS", true);

    // Send a chat message via lobby chat
    for(unsigned i = 0; i < 3; i++)
    {
        MOCK_EXPECT(lobby.LC_Chat).once().with("OtherPlayer", "Test").in(s);
        lobbyServer.connections[0].sendQueue.push(new LobbyMessage_Chat("OtherPlayer", "Test"));

        for(unsigned j = 0; j < 50; j++)
            run();
    }
}

BOOST_AUTO_TEST_SUITE_END()
