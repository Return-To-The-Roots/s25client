// Copyright (C) 2005 - 2022 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "JoinPlayerInfo.h"
#include "RttrConfig.h"
#include "TestServer.h"
#include "network/ClientInterface.h"
#include "network/GameClient.h"
#include "network/GameMessage.h"
#include "network/GameMessages.h"
#include "gameTypes/GameTypesOutput.h"
#include "test/testConfig.h"
#include "rttr/test/random.hpp"
#include "s25util/boostTestHelpers.h"
#include "s25util/tmpFile.h"
#include <turtle/mock.hpp>
#include <boost/nowide/cstdio.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/test/unit_test.hpp>

// LCOV_EXCL_START
BOOST_TEST_DONT_PRINT_LOG_VALUE(ClientState)
// LCOV_EXCL_STOP

namespace {
MOCK_BASE_CLASS(MockClientInterface, ClientInterface)
{
    // LCOV_EXCL_START
    MOCK_METHOD(CI_NextConnectState, 1)
    MOCK_METHOD(CI_MapPartReceived, 2)
    MOCK_METHOD(CI_Error, 1)
    MOCK_METHOD(CI_NewPlayer, 1)
    MOCK_METHOD(CI_PlayerLeft, 1)
    MOCK_METHOD(CI_GameLoading, 1)
    MOCK_METHOD(CI_GameStarted, 0)
    MOCK_METHOD(CI_PlayerDataChanged, 1)
    MOCK_METHOD(CI_PingChanged, 2)
    MOCK_METHOD(CI_ReadyChanged, 2)
    MOCK_METHOD(CI_PlayersSwapped, 2)
    MOCK_METHOD(CI_GGSChanged, 1)
    // LCOV_EXCL_STOP
};

} // namespace

BOOST_AUTO_TEST_CASE(ClientFollowsConnectProtocol)
{
    GameClient client;
    GameMessageInterface& clientMsgInterface = client;
    MockClientInterface callbacks;
    client.SetInterface(&callbacks);
    TestServer server;
    const auto serverPort = server.tryListen();
    BOOST_TEST_REQUIRE(serverPort >= 0);
    const auto pw = rttr::test::randString(10);
    const auto serverType = rttr::test::randomEnum<ServerType>();
    MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::Initiated).once();
    BOOST_TEST_REQUIRE(client.Connect("localhost", pw, serverType, serverPort, false, false));
    BOOST_TEST_REQUIRE(client.GetState() == ClientState::Connect);
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::VerifyServer).once();
        clientMsgInterface.OnGameMessage(GameMessage_Player_Id(1));
        const auto msg = boost::dynamic_pointer_cast<GameMessage_Server_Type>(client.GetMainPlayer().sendQueue.pop());
        BOOST_TEST_REQUIRE(msg);
        BOOST_TEST(msg->type == serverType);
    }
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::QueryPw).once();
        clientMsgInterface.OnGameMessage(GameMessage_Server_TypeOK(GameMessage_Server_TypeOK::StatusCode::Ok, ""));
        const auto msg =
          boost::dynamic_pointer_cast<GameMessage_Server_Password>(client.GetMainPlayer().sendQueue.pop());
        BOOST_TEST_REQUIRE(msg);
        BOOST_TEST(msg->password == pw);
    }
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::QueryMapInfo).once();
        clientMsgInterface.OnGameMessage(GameMessage_Server_Password("true"));
        BOOST_TEST_REQUIRE(dynamic_cast<GameMessage_Player_Name*>(client.GetMainPlayer().sendQueue.pop().get()));
        const auto msg = boost::dynamic_pointer_cast<GameMessage_MapRequest>(client.GetMainPlayer().sendQueue.pop());
        BOOST_TEST_REQUIRE(msg);
        BOOST_TEST(msg->requestInfo);
    }
    const boost::filesystem::path testMapPath =
      rttr::test::rttrBaseDir / "tests" / "testData" / "maps" / "LuaFunctions.SWD";
    TmpFile mapFile;
    mapFile.close();
    MapInfo mapInfo;
    mapInfo.mapData.CompressFromFile(testMapPath, &mapInfo.mapChecksum);
    boost::nowide::remove(mapFile.filePath.string().c_str());
    RTTRCONFIG.overridePathMapping("RTTR_USERDATA", mapFile.filePath.parent_path());
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::ReceiveMap).once();
        clientMsgInterface.OnGameMessage(GameMessage_Map_Info(mapFile.filePath.filename().string(), MapType::OldMap,
                                                              mapInfo.mapData.uncompressedLength,
                                                              mapInfo.mapData.data.size(), 0, 0));
        const auto msg = boost::dynamic_pointer_cast<GameMessage_MapRequest>(client.GetMainPlayer().sendQueue.pop());
        BOOST_TEST_REQUIRE(msg);
        BOOST_TEST(!msg->requestInfo);
    }
    {
        BOOST_TEST_REQUIRE(mapInfo.mapData.data.size() > 10u); // Should be or we can't chunk the map
        MOCK_EXPECT(callbacks.CI_MapPartReceived).with(10u, mapInfo.mapData.data.size()).once();
        clientMsgInterface.OnGameMessage(GameMessage_Map_Data(true, 0, mapInfo.mapData.data.data(), 10));
        BOOST_TEST(client.GetMainPlayer().sendQueue.empty());

        MOCK_EXPECT(callbacks.CI_MapPartReceived).with(mapInfo.mapData.data.size(), mapInfo.mapData.data.size()).once();
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::VerifyMap).once();
        clientMsgInterface.OnGameMessage(
          GameMessage_Map_Data(true, 10, mapInfo.mapData.data.data() + 10, mapInfo.mapData.data.size() - 10));
        const auto msg = boost::dynamic_pointer_cast<GameMessage_Map_Checksum>(client.GetMainPlayer().sendQueue.pop());
        BOOST_TEST_REQUIRE(msg);
        BOOST_TEST(msg->mapChecksum == mapInfo.mapChecksum);
        BOOST_TEST(msg->luaChecksum == 0u);
    }
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::QueryServerName).once();
        clientMsgInterface.OnGameMessage(GameMessage_Map_ChecksumOK(true, false));
        BOOST_TEST(client.GetMainPlayer().sendQueue.empty());
    }
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::QueryPlayerList).once();
        clientMsgInterface.OnGameMessage(GameMessage_Server_Name(rttr::test::randString(10)));
        BOOST_TEST(client.GetMainPlayer().sendQueue.empty());
    }
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::QuerySettings).once();
        clientMsgInterface.OnGameMessage(GameMessage_Player_List(std::vector<JoinPlayerInfo>(3)));
        BOOST_TEST(client.GetMainPlayer().sendQueue.empty());
    }
    {
        BOOST_TEST_REQUIRE(client.GetState() == ClientState::Connect);
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::Finished).once();
        clientMsgInterface.OnGameMessage(GameMessage_GGSChange());
        BOOST_TEST(client.GetMainPlayer().sendQueue.empty());
        // And done
        BOOST_TEST_REQUIRE(client.GetState() == ClientState::Config);
    }
}
