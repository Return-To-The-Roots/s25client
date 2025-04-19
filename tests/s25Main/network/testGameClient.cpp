// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "JoinPlayerInfo.h"
#include "RttrConfig.h"
#include "TestServer.h"
#include "files.h"
#include "network/ClientInterface.h"
#include "network/GameClient.h"
#include "network/GameMessage.h"
#include "network/GameMessages.h"
#include "gameTypes/GameTypesOutput.h"
#include "test/testConfig.h"
#include "rttr/test/ConfigOverride.hpp"
#include "rttr/test/LogAccessor.hpp"
#include "rttr/test/random.hpp"
#include "s25util/boostTestHelpers.h"
#include "s25util/tmpFile.h"
#include <turtle/mock.hpp>
#include <boost/filesystem.hpp>
#include <boost/nowide/cstdio.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

namespace bfs = boost::filesystem;

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

class CustomUserMapFolderFixture : rttr::test::ConfigOverride
{
public:
    CustomUserMapFolderFixture() : ConfigOverride("USERDATA", rttr::test::rttrTestDataDirOut)
    {
        bfs::create_directories(RTTRCONFIG.ExpandPath(s25::folders::mapsPlayed));
    }
};

} // namespace

BOOST_FIXTURE_TEST_SUITE(GameClientTests, CustomUserMapFolderFixture)

static constexpr std::array<bool, 2> usesLuaScriptValues{false, true};
BOOST_DATA_TEST_CASE(ClientFollowsConnectProtocol, usesLuaScriptValues, usesLuaScript)
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
    const boost::filesystem::path testLuaPath = boost::filesystem::path(testMapPath).replace_extension("lua");
    MapInfo mapInfo;
    mapInfo.mapData.CompressFromFile(testMapPath, &mapInfo.mapChecksum);
    if(usesLuaScript)
        mapInfo.luaData.CompressFromFile(testLuaPath, &mapInfo.luaChecksum);
    const auto mapDataSize = mapInfo.mapData.data.size();
    const auto luaDataSize = (usesLuaScript) ? mapInfo.luaData.data.size() : 0u;
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::ReceiveMap).once();
        clientMsgInterface.OnGameMessage(GameMessage_Map_Info(testMapPath.filename().string(), MapType::OldMap,
                                                              mapInfo.mapData.uncompressedLength, mapDataSize,
                                                              mapInfo.luaData.uncompressedLength, luaDataSize));
        const auto msg = boost::dynamic_pointer_cast<GameMessage_MapRequest>(client.GetMainPlayer().sendQueue.pop());
        BOOST_TEST_REQUIRE(msg);
        BOOST_TEST(!msg->requestInfo);
    }
    {
        const auto totalSize = mapDataSize + luaDataSize;
        BOOST_TEST_REQUIRE(mapDataSize > 10u); // Should be or we can't chunk the map
        if(usesLuaScript)
            BOOST_TEST_REQUIRE(luaDataSize > 5u); // Should be or we can't chunk the lua data
        // First part of map
        MOCK_EXPECT(callbacks.CI_MapPartReceived).with(10u, totalSize).once();
        clientMsgInterface.OnGameMessage(GameMessage_Map_Data(true, 0, mapInfo.mapData.data.data(), 10));
        BOOST_TEST(client.GetMainPlayer().sendQueue.empty());
        // Remaining part of map
        MOCK_EXPECT(callbacks.CI_MapPartReceived).with(mapDataSize, totalSize).once();
        if(!usesLuaScript)
            MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::VerifyMap).once();
        clientMsgInterface.OnGameMessage(
          GameMessage_Map_Data(true, 10, mapInfo.mapData.data.data() + 10, mapDataSize - 10));
        if(usesLuaScript)
        {
            // First part of lua
            MOCK_EXPECT(callbacks.CI_MapPartReceived).with(mapDataSize + 5u, totalSize).once();
            clientMsgInterface.OnGameMessage(GameMessage_Map_Data(false, 0, mapInfo.luaData.data.data(), 5));
            BOOST_TEST(client.GetMainPlayer().sendQueue.empty());
            // Remaining part of lua -> Transmission done
            MOCK_EXPECT(callbacks.CI_MapPartReceived).with(totalSize, totalSize).once();
            MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::VerifyMap).once();
            clientMsgInterface.OnGameMessage(
              GameMessage_Map_Data(false, 5, mapInfo.luaData.data.data() + 5, luaDataSize - 5));
        }

        const auto msg = boost::dynamic_pointer_cast<GameMessage_Map_Checksum>(client.GetMainPlayer().sendQueue.pop());
        BOOST_TEST_REQUIRE(msg);
        BOOST_TEST(msg->mapChecksum == mapInfo.mapChecksum);
        BOOST_TEST(msg->luaChecksum == mapInfo.luaChecksum);
        BOOST_TEST(bfs::exists(RTTRCONFIG.ExpandPath(s25::folders::mapsPlayed) / testMapPath.filename()));
        if(usesLuaScript)
            BOOST_TEST(bfs::exists(RTTRCONFIG.ExpandPath(s25::folders::mapsPlayed) / testLuaPath.filename()));
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

BOOST_AUTO_TEST_CASE(ClientDetectsMapBufferOverflow)
{
    rttr::test::LogAccessor _suppressLogOutput;
    GameClient client;
    GameMessageInterface& clientMsgInterface = client;
    MockClientInterface callbacks;
    client.SetInterface(&callbacks);
    TestServer server;
    const auto serverPort = server.tryListen();
    BOOST_TEST_REQUIRE(serverPort >= 0);
    const auto pw = rttr::test::randString(10);
    const auto serverType = rttr::test::randomEnum<ServerType>();
    mock::sequence s;
    MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::Initiated).once();
    MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::VerifyServer).once();
    MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::QueryPw).once();
    MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::QueryMapInfo).once();
    MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::ReceiveMap).once();

    BOOST_TEST_REQUIRE(client.Connect("localhost", pw, serverType, serverPort, false, false));
    clientMsgInterface.OnGameMessage(GameMessage_Player_Id(1));
    clientMsgInterface.OnGameMessage(GameMessage_Server_TypeOK(GameMessage_Server_TypeOK::StatusCode::Ok, ""));
    clientMsgInterface.OnGameMessage(GameMessage_Server_Password("true"));

    constexpr auto chunkSize = 10u;
    const auto mapDataSize = rttr::test::randomValue(2 * chunkSize, 10 * chunkSize);      // At least 2 chunks
    const auto uncompressedSize = rttr::test::randomValue(mapDataSize, 10 * mapDataSize); // Doesn't really matter
    std::vector<char> mapData(mapDataSize);
    clientMsgInterface.OnGameMessage(
      GameMessage_Map_Info("testMap.swd", MapType::OldMap, uncompressedSize, mapDataSize, 0, 0));
    // First part of map
    MOCK_EXPECT(callbacks.CI_MapPartReceived).in(s).with(chunkSize, mapDataSize).once();
    clientMsgInterface.OnGameMessage(GameMessage_Map_Data(true, 0, mapData.data(), chunkSize));
    BOOST_TEST_REQUIRE(mock::verify());
    BOOST_TEST(client.GetState() == ClientState::Connect);

    // Remaining part of map but to big/wrong offset
    MOCK_EXPECT(callbacks.CI_Error).with(ClientError::MapTransmission).once();
    const auto remainingSize = mapDataSize - chunkSize;
    clientMsgInterface.OnGameMessage(GameMessage_Map_Data(true, chunkSize, mapData.data(), remainingSize + 1u));
    BOOST_TEST(client.GetState() == ClientState::Stopped);
}

BOOST_AUTO_TEST_SUITE_END()
