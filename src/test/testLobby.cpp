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

#include "rttrDefines.h" // IWYU pragma: keep
#include "GameLobby.h"
#include "JoinPlayerInfo.h"
#include "TestServer.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlOptionGroup.h"
#include "desktops/dskHostGame.h"
#include "helpers/Deleter.h"
#include "initTestHelpers.h"
#include "liblobby/LobbyClient.h"
#include "liblobby/LobbyMessages.h"
#include <boost/foreach.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/test/unit_test.hpp>

struct TestLobbySever : public TestServer, public LobbyMessageInterface
{
    std::string testUser, testPw;
    boost::ptr_vector<LobbyMessage> messages;
    TestLobbySever() : testUser("testUser"), testPw("testPw") {}

    void handleMessages() override
    {
        for(unsigned id = 0; id < connections.size(); ++id)
        {
            Connection& con = connections[id];
            while(!con.recvQueue.empty())
            {
                boost::interprocess::unique_ptr<Message, Deleter<Message> > msg(con.recvQueue.popFront());
                if(!msg->run(this, id))
                {
                    LobbyMessage* lobbyMsg = dynamic_cast<LobbyMessage*>(msg.get());
                    if(lobbyMsg)
                    {
                        messages.push_back(lobbyMsg);
                        msg.release();
                    }
                }
            }
        }
    }

    Connection acceptConnection(unsigned id, const Socket& so) override
    {
        Connection result(LobbyMessage::create_lobby, so);
        result.sendQueue.push(new LobbyMessage_Id(id));
        return result;
    }

    bool OnNMSLobbyLogin(unsigned id, const unsigned revision, const std::string& user, const std::string& pass,
                         const std::string& version) override
    {
        BOOST_REQUIRE_EQUAL(user, testUser);
        BOOST_REQUIRE_EQUAL(pass, testPw);
        connections[id].sendQueue.push(new LobbyMessage_Login_Done("Foo@Bar.Foo"));
        return true;
    }
};

struct LobbyFixture
{
    TestLobbySever lobbyServer;
    uint16_t lobbyPort;
    LobbyFixture() : lobbyPort(5664) { BOOST_REQUIRE(lobbyServer.listen(lobbyPort)); }
    void run()
    {
        lobbyServer.run();
        LOBBYCLIENT.Run();
    }
};

BOOST_FIXTURE_TEST_SUITE(Lobby, LobbyFixture)

void deleteNoting(void*) {}

BOOST_AUTO_TEST_CASE(LobbyChat)
{
    initGUITests();
    BOOST_REQUIRE(LOBBYCLIENT.Login("localhost", lobbyPort, lobbyServer.testUser, lobbyServer.testPw, false));
    for(unsigned i = 0; i < 50; i++)
    {
        run();
        if(LOBBYCLIENT.IsLoggedIn())
            break;
    }
    BOOST_REQUIRE(LOBBYCLIENT.IsLoggedIn());
    GameLobby gameLobby(false, true, 2);
    JoinPlayerInfo& player = gameLobby.getPlayer(0);
    player.ps = PS_OCCUPIED;
    player.name = "TestName";
    player.isHost = true;

    dskHostGame* desktop = new dskHostGame(ServerType::LOBBY, boost::shared_ptr<GameLobby>(&gameLobby, &deleteNoting), 0);
    ClientInterface* ci = static_cast<ClientInterface*>(desktop);
    std::vector<ctrlOptionGroup*> chatTab = desktop->GetCtrls<ctrlOptionGroup>();
    BOOST_REQUIRE_EQUAL(chatTab.size(), 1u);
    std::vector<ctrlButton*> chatBts = chatTab.front()->GetCtrls<ctrlButton>();
    BOOST_REQUIRE_EQUAL(chatBts.size(), 2u);
    WINDOWMANAGER.Switch(desktop);
    WINDOWMANAGER.Draw();

    // Send a chat message via lobby chat and game chat with either visible
    for(unsigned i = 0; i < 3; i++)
    {
        lobbyServer.connections[0].sendQueue.push(new LobbyMessage_Chat("OtherPlayer", "Test"));
        ci->CI_Chat(0, CD_ALL, "Test2");
        run();
        static_cast<Window*>(desktop)->Msg_OptionGroupChange(chatTab.front()->GetID(), chatBts[i % 2]->GetID());
    }
}

BOOST_AUTO_TEST_SUITE_END()
