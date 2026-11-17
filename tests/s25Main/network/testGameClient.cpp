// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "JoinPlayerInfo.h"
#include "RttrConfig.h"
#include "TestServer.h"
#include "files.h"
#include "helpers/chronoIO.h"
#include "network/ClientInterface.h"
#include "network/GameClient.h"
#include "network/GameMessage.h"
#include "network/GameMessages.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameData/GameConsts.h"
#include "test/testConfig.h"
#include "rttr/test/ConfigOverride.hpp"
#include "rttr/test/LogAccessor.hpp"
#include "rttr/test/TmpFolder.hpp"
#include "rttr/test/random.hpp"
#include "s25util/boostTestHelpers.h"
#include "s25util/tmpFile.h"
#include <turtle/mock.hpp>
#include <boost/filesystem.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/list.hpp>
#include <boost/nowide/cstdio.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

namespace bfs = boost::filesystem;
namespace dataset = boost::unit_test::data;

// LCOV_EXCL_START
namespace boost::test_tools::tt_detail {
template<class T, class R>
struct print_log_value<std::chrono::duration<T, R>>
{
    void operator()(std::ostream& out, const std::chrono::duration<T, R>& value) { out << helpers::withUnit(value); }
};
} // namespace boost::test_tools::tt_detail
BOOST_TEST_DONT_PRINT_LOG_VALUE(ClientState)
static std::ostream& operator<<(std::ostream& os, const ConnectState& state)
{
    return os << rttr::enumCast(state);
}
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

class CustomUserMapFolderFixture
{
    rttr::test::TmpFolder tmpFolder_;
    rttr::test::ConfigOverride parent_;

public:
    // We need an empty folder for each test to avoid reusing maps downloaded in previous tests
    // which breaks expected flow of events when this map is reused.
    CustomUserMapFolderFixture() : parent_("USERDATA", tmpFolder_)
    {
        bfs::create_directories(RTTRCONFIG.expandPath(s25::folders::mapsPlayed));
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
    client.setInterface(&callbacks);
    TestServer server;
    const auto serverPort = server.tryListen();
    BOOST_TEST_REQUIRE(serverPort >= 0);
    const auto pw = rttr::test::randString(10);
    const auto serverType = rttr::test::randomEnum<ServerType>();
    MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::Initiated).once();
    BOOST_TEST_REQUIRE(client.connect("localhost", pw, serverType, serverPort, false, false));
    BOOST_TEST_REQUIRE(client.getState() == ClientState::Connect);
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::VerifyServer).once();
        clientMsgInterface.onGameMessage(GameMessage_Player_Id(1));
        const auto msg = boost::dynamic_pointer_cast<GameMessage_Server_Type>(client.getMainPlayer().sendQueue.pop());
        BOOST_TEST_REQUIRE(msg);
        BOOST_TEST(msg->type == serverType);
    }
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::QueryPw).once();
        clientMsgInterface.onGameMessage(GameMessage_Server_TypeOK(GameMessage_Server_TypeOK::StatusCode::Ok, ""));
        const auto msg =
          boost::dynamic_pointer_cast<GameMessage_Server_Password>(client.getMainPlayer().sendQueue.pop());
        BOOST_TEST_REQUIRE(msg);
        BOOST_TEST(msg->password == pw);
    }
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::QueryMapInfo).once();
        clientMsgInterface.onGameMessage(GameMessage_Server_Password("true"));
        BOOST_TEST_REQUIRE(dynamic_cast<GameMessage_Player_Name*>(client.getMainPlayer().sendQueue.pop().get()));
        BOOST_TEST_REQUIRE(dynamic_cast<GameMessage_Player_Portrait*>(client.getMainPlayer().sendQueue.pop().get()));
        const auto msg = boost::dynamic_pointer_cast<GameMessage_MapRequest>(client.getMainPlayer().sendQueue.pop());
        BOOST_TEST_REQUIRE(msg);
        BOOST_TEST(msg->requestInfo);
    }
    const boost::filesystem::path testMapPath =
      rttr::test::rttrBaseDir / "tests" / "testData" / "maps" / "LuaFunctions.SWD";
    const boost::filesystem::path testLuaPath = boost::filesystem::path(testMapPath).replace_extension("lua");
    MapInfo mapInfo;
    mapInfo.mapData.compressFromFile(testMapPath, &mapInfo.mapChecksum);
    if(usesLuaScript)
        mapInfo.luaData.compressFromFile(testLuaPath, &mapInfo.luaChecksum);
    const auto mapDataSize = mapInfo.mapData.data.size();
    const auto luaDataSize = (usesLuaScript) ? mapInfo.luaData.data.size() : 0u;
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::ReceiveMap).once();
        clientMsgInterface.onGameMessage(GameMessage_Map_Info(testMapPath.filename().string(), MapType::OldMap,
                                                              mapInfo.mapData.uncompressedLength, mapDataSize,
                                                              mapInfo.luaData.uncompressedLength, luaDataSize));
        const auto msg = boost::dynamic_pointer_cast<GameMessage_MapRequest>(client.getMainPlayer().sendQueue.pop());
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
        clientMsgInterface.onGameMessage(GameMessage_Map_Data(true, 0, mapInfo.mapData.data.data(), 10));
        BOOST_TEST(client.getMainPlayer().sendQueue.empty());
        // Remaining part of map
        MOCK_EXPECT(callbacks.CI_MapPartReceived).with(mapDataSize, totalSize).once();
        if(!usesLuaScript)
            MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::VerifyMap).once();
        clientMsgInterface.onGameMessage(
          GameMessage_Map_Data(true, 10, mapInfo.mapData.data.data() + 10, mapDataSize - 10));
        if(usesLuaScript)
        {
            // First part of lua
            MOCK_EXPECT(callbacks.CI_MapPartReceived).with(mapDataSize + 5u, totalSize).once();
            clientMsgInterface.onGameMessage(GameMessage_Map_Data(false, 0, mapInfo.luaData.data.data(), 5));
            BOOST_TEST(client.getMainPlayer().sendQueue.empty());
            // Remaining part of lua -> Transmission done
            MOCK_EXPECT(callbacks.CI_MapPartReceived).with(totalSize, totalSize).once();
            MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::VerifyMap).once();
            clientMsgInterface.onGameMessage(
              GameMessage_Map_Data(false, 5, mapInfo.luaData.data.data() + 5, luaDataSize - 5));
        }

        const auto msg = boost::dynamic_pointer_cast<GameMessage_Map_Checksum>(client.getMainPlayer().sendQueue.pop());
        BOOST_TEST_REQUIRE(msg);
        BOOST_TEST(msg->mapChecksum == mapInfo.mapChecksum);
        BOOST_TEST(msg->luaChecksum == mapInfo.luaChecksum);
        BOOST_TEST(bfs::exists(RTTRCONFIG.expandPath(s25::folders::mapsPlayed) / testMapPath.filename()));
        if(usesLuaScript)
            BOOST_TEST(bfs::exists(RTTRCONFIG.expandPath(s25::folders::mapsPlayed) / testLuaPath.filename()));
    }
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::QueryServerName).once();
        clientMsgInterface.onGameMessage(GameMessage_Map_ChecksumOK(true, false));
        BOOST_TEST(client.getMainPlayer().sendQueue.empty());
    }
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::QueryPlayerList).once();
        clientMsgInterface.onGameMessage(GameMessage_Server_Name(rttr::test::randString(10)));
        BOOST_TEST(client.getMainPlayer().sendQueue.empty());
    }
    {
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::QuerySettings).once();
        clientMsgInterface.onGameMessage(GameMessage_Player_List(std::vector<JoinPlayerInfo>(3)));
        BOOST_TEST(client.getMainPlayer().sendQueue.empty());
    }
    {
        BOOST_TEST_REQUIRE(client.getState() == ClientState::Connect);
        MOCK_EXPECT(callbacks.CI_NextConnectState).with(ConnectState::Finished).once();
        clientMsgInterface.onGameMessage(GameMessage_GGSChange());
        BOOST_TEST(client.getMainPlayer().sendQueue.empty());
        // And done
        BOOST_TEST_REQUIRE(client.getState() == ClientState::Config);
    }

    // Reuse existing map (stored after previous connect)
    {
        mock::sequence s;
        MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::Initiated).once();
        MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::VerifyServer).once();
        MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::QueryPw).once();
        MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::QueryMapInfo).once();
        // Skip ReceiveMap
        MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::VerifyMap).once();
        MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::QueryServerName).once();
        MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::QueryPlayerList).once();
        MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::QuerySettings).once();
        MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::Finished).once();

        client.stop();
        BOOST_TEST_REQUIRE(client.connect("localhost", pw, serverType, serverPort, false, false));
        clientMsgInterface.onGameMessage(GameMessage_Player_Id(1));
        clientMsgInterface.onGameMessage(GameMessage_Server_TypeOK(GameMessage_Server_TypeOK::StatusCode::Ok, ""));
        clientMsgInterface.onGameMessage(GameMessage_Server_Password("true"));
        clientMsgInterface.onGameMessage(GameMessage_Map_Info(testMapPath.filename().string(), MapType::OldMap,
                                                              mapInfo.mapData.uncompressedLength, mapDataSize,
                                                              mapInfo.luaData.uncompressedLength, luaDataSize));

        using msg_types =
          boost::mpl::list<GameMessage_Server_Type, GameMessage_Server_Password, GameMessage_Player_Name,
                           GameMessage_Player_Portrait, GameMessage_MapRequest>;
        boost::mpl::for_each<msg_types>([&client](auto arg) {
            using msg_type = decltype(arg);
            auto msg = client.getMainPlayer().sendQueue.pop();
            BOOST_TEST_REQUIRE(boost::dynamic_pointer_cast<msg_type>(std::move(msg)));
        });
        const auto msg = boost::dynamic_pointer_cast<GameMessage_Map_Checksum>(client.getMainPlayer().sendQueue.pop());
        BOOST_TEST_REQUIRE(msg);
        BOOST_TEST(msg->mapChecksum == mapInfo.mapChecksum);
        BOOST_TEST(msg->luaChecksum == mapInfo.luaChecksum);
        // Remaining packets
        clientMsgInterface.onGameMessage(GameMessage_Map_ChecksumOK(true, false));
        clientMsgInterface.onGameMessage(GameMessage_Server_Name(rttr::test::randString(10)));
        clientMsgInterface.onGameMessage(GameMessage_Player_List(std::vector<JoinPlayerInfo>(3)));
        clientMsgInterface.onGameMessage(GameMessage_GGSChange());
        BOOST_TEST_REQUIRE(client.getState() == ClientState::Config);
    }
}

BOOST_AUTO_TEST_CASE(ClientStoresReceivedSavegamesInSaveFolder)
{
    rttr::test::TmpFolder testUserData;
    rttr::test::ConfigOverride userDataOverride("USERDATA", testUserData);

    GameClient client;
    GameMessageInterface& clientMsgInterface = client;
    TestServer server;
    const auto serverPort = server.tryListen();
    BOOST_TEST_REQUIRE(serverPort >= 0);

    BOOST_TEST_REQUIRE(client.connect("localhost", rttr::test::randString(10), rttr::test::randomEnum<ServerType>(),
                                      serverPort, false, false));
    clientMsgInterface.onGameMessage(GameMessage_Player_Id(1));
    client.getMainPlayer().sendQueue.clear();
    clientMsgInterface.onGameMessage(GameMessage_Server_TypeOK(GameMessage_Server_TypeOK::StatusCode::Ok, ""));
    client.getMainPlayer().sendQueue.clear();
    clientMsgInterface.onGameMessage(GameMessage_Server_Password("true"));
    client.getMainPlayer().sendQueue.clear();

    const auto expectedSavePath = RTTRCONFIG.expandPath(s25::folders::save) / "received.sav";

    clientMsgInterface.onGameMessage(GameMessage_Map_Info("received.sav", MapType::Savegame, 1, 1, 0, 0));
    const auto msg = boost::dynamic_pointer_cast<GameMessage_MapRequest>(client.getMainPlayer().sendQueue.pop());
    BOOST_TEST_REQUIRE(msg);
    BOOST_TEST(!msg->requestInfo);
    BOOST_TEST(client.getMapType() == MapType::Savegame);
    BOOST_TEST(client.getMapPath() == expectedSavePath);
}

static constexpr std::array allServerTypes{ServerType::Local, ServerType::LAN, ServerType::Direct, ServerType::Lobby};
static constexpr std::array allMapTypes{MapType::Savegame, MapType::OldMap};
#ifndef __INTELLISENSE__
BOOST_DATA_TEST_CASE(ClientTreatsLocalSavegameFilenameVerbatim,
                     dataset::make(allServerTypes) * dataset::make(allMapTypes), serverType, mapType)
#else
void ClientTreatsLocalSavegameFilenameVerbatim(ServerType serverType, MapType mapType)
#endif
{
    rttr::test::TmpFolder testUserData;
    rttr::test::ConfigOverride userDataOverride("USERDATA", testUserData);

    GameClient client;
    GameMessageInterface& clientMsgInterface = client;
    TestServer server;
    const auto serverPort = server.tryListen();
    BOOST_TEST_REQUIRE(serverPort >= 0);

    BOOST_TEST_REQUIRE(client.connect("localhost", rttr::test::randString(10), serverType, serverPort, false, false));
    clientMsgInterface.onGameMessage(GameMessage_Player_Id(1));
    client.getMainPlayer().sendQueue.clear();
    clientMsgInterface.onGameMessage(GameMessage_Server_TypeOK(GameMessage_Server_TypeOK::StatusCode::Ok, ""));
    client.getMainPlayer().sendQueue.clear();
    clientMsgInterface.onGameMessage(GameMessage_Server_Password("true"));
    client.getMainPlayer().sendQueue.clear();

    // Only local games (client and server are the same process via loopback) read the savegame
    // straight off disk, so the filename is already a valid name for the local filesystem and must
    // not be re-sanitized - doing so anyway would create a renamed duplicate next to the original.
    // The bypass requires *both* ServerType::Local and MapType::Savegame - any other combination
    // (e.g. a normal map, which is always copied into a separate "played maps" cache folder) must
    // still go through the usual sanitization.
    const std::string rawName = (mapType == MapType::Savegame) ? "my save.sav" : "my map.swd";
    clientMsgInterface.onGameMessage(GameMessage_Map_Info(rawName, mapType, 1, 1, 0, 0));

    const bool expectVerbatim = (serverType == ServerType::Local && mapType == MapType::Savegame);
    const std::string sanitizedName = (mapType == MapType::Savegame) ? "my_save.sav" : "my_map.swd";
    const auto targetFolder = (mapType == MapType::Savegame) ? s25::folders::save : s25::folders::mapsPlayed;
    BOOST_TEST(client.getMapPath() == RTTRCONFIG.expandPath(targetFolder) / (expectVerbatim ? rawName : sanitizedName));
}

BOOST_AUTO_TEST_CASE(ClientDetectsMapBufferOverflow)
{
    rttr::test::LogAccessor _suppressLogOutput;
    GameClient client;
    GameMessageInterface& clientMsgInterface = client;
    MockClientInterface callbacks;
    client.setInterface(&callbacks);
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

    BOOST_TEST_REQUIRE(client.connect("localhost", pw, serverType, serverPort, false, false));
    clientMsgInterface.onGameMessage(GameMessage_Player_Id(1));
    clientMsgInterface.onGameMessage(GameMessage_Server_TypeOK(GameMessage_Server_TypeOK::StatusCode::Ok, ""));
    clientMsgInterface.onGameMessage(GameMessage_Server_Password("true"));

    constexpr auto chunkSize = 10u;
    const auto mapDataSize = rttr::test::randomValue(2 * chunkSize, 10 * chunkSize);      // At least 2 chunks
    const auto uncompressedSize = rttr::test::randomValue(mapDataSize, 10 * mapDataSize); // Doesn't really matter
    std::vector<char> mapData(mapDataSize);
    clientMsgInterface.onGameMessage(
      GameMessage_Map_Info("testMap.swd", MapType::OldMap, uncompressedSize, mapDataSize, 0, 0));
    // First part of map
    MOCK_EXPECT(callbacks.CI_MapPartReceived).in(s).with(chunkSize, mapDataSize).once();
    clientMsgInterface.onGameMessage(GameMessage_Map_Data(true, 0, mapData.data(), chunkSize));
    BOOST_TEST_REQUIRE(mock::verify());
    BOOST_TEST(client.getState() == ClientState::Connect);

    // Remaining part of map but to big/wrong offset
    MOCK_EXPECT(callbacks.CI_Error).with(ClientError::MapTransmission).once();
    const auto remainingSize = mapDataSize - chunkSize;
    clientMsgInterface.onGameMessage(GameMessage_Map_Data(true, chunkSize, mapData.data(), remainingSize + 1u));
    BOOST_TEST(client.getState() == ClientState::Stopped);
}

BOOST_AUTO_TEST_CASE(ClientReportsWrongPasswordResponse)
{
    GameClient client;
    GameMessageInterface& clientMsgInterface = client;
    MockClientInterface callbacks;
    client.setInterface(&callbacks);
    TestServer server;
    const auto serverPort = server.tryListen();
    BOOST_TEST_REQUIRE(serverPort >= 0);
    const auto serverType = rttr::test::randomEnum<ServerType>();
    mock::sequence s;
    MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::Initiated).once();
    MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::VerifyServer).once();
    MOCK_EXPECT(callbacks.CI_NextConnectState).in(s).with(ConnectState::QueryPw).once();
    MOCK_EXPECT(callbacks.CI_Error).in(s).with(ClientError::WrongPassword).once();

    BOOST_TEST_REQUIRE(client.connect("localhost", rttr::test::randString(10), serverType, serverPort, false, false));
    clientMsgInterface.onGameMessage(GameMessage_Player_Id(1));
    clientMsgInterface.onGameMessage(GameMessage_Server_TypeOK(GameMessage_Server_TypeOK::StatusCode::Ok, ""));
    BOOST_TEST_REQUIRE(boost::dynamic_pointer_cast<GameMessage_Server_Type>(client.getMainPlayer().sendQueue.pop()));
    BOOST_TEST_REQUIRE(
      boost::dynamic_pointer_cast<GameMessage_Server_Password>(client.getMainPlayer().sendQueue.pop()));

    clientMsgInterface.onGameMessage(GameMessage_Server_Password("false"));

    BOOST_TEST(client.getState() == ClientState::Stopped);
}

using namespace std::chrono_literals;

BOOST_AUTO_TEST_CASE(CanSetNewSpeed)
{
    GameClient client;

    client.setNewSpeed(50ms);
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 50ms);
    client.setNewSpeed(40ms);
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 40ms);
}

BOOST_AUTO_TEST_CASE(IncreaseSpeed_DecreasesGF_WhenDivisibleBy10_By10)
{
    GameClient client;

    client.setNewSpeed(50ms);
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 50ms);
    client.increaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 40ms);
    client.increaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 30ms);
    client.increaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 20ms);
    client.increaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 10ms);
}

BOOST_AUTO_TEST_CASE(DecreaseSpeed_IncreasesGF_WhenDivisible10_By10)
{
    GameClient client;

    client.setNewSpeed(10ms);
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 10ms);
    client.decreaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 20ms);
    client.decreaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 30ms);
    client.decreaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 40ms);
    client.decreaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 50ms);
}

BOOST_AUTO_TEST_CASE(IncreaseSpeed_DecreasesGF_WhenNotDivisibleBy10_RoundsDownTo10)
{
    GameClient client;

    client.setNewSpeed(25ms);
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 25ms);
    client.increaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 20ms);

    client.setNewSpeed(12ms);
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 12ms);
    client.increaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 10ms);
}

BOOST_AUTO_TEST_CASE(DecreaseSpeed_IncreasesGF_WhenNotDivisibleBy10_RoundsUpTo10)
{
    GameClient client;

    client.setNewSpeed(25ms);
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 25ms);
    client.decreaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 30ms);

    client.setNewSpeed(12ms);
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 12ms);
    client.decreaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == 20ms);
}

BOOST_AUTO_TEST_CASE(IncreaseSpeed_WrapsAround_OrCaps)
{
    GameClient client;

    client.setNewSpeed(MAX_SPEED + 2ms);
    client.increaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MAX_SPEED);
    client.increaseSpeed();
#ifndef NDEBUG
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MAX_SPEED_DEBUG);
    client.increaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MAX_SPEED_DEBUG);
#else
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MAX_SPEED);
#endif
    // Wrap around if request
    client.increaseSpeed(true);
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MIN_SPEED);
}

BOOST_AUTO_TEST_CASE(DecreaseSpeed_CapsAtMinSpeed)
{
    GameClient client;

    client.setNewSpeed(MIN_SPEED - 2ms);
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == (MIN_SPEED - 2ms));
    client.decreaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MIN_SPEED);
    client.decreaseSpeed();
#ifndef NDEBUG
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MIN_SPEED_DEBUG);
    client.decreaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MIN_SPEED_DEBUG);
#else
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MIN_SPEED);
#endif
}

#ifndef NDEBUG
BOOST_AUTO_TEST_CASE(ChangeSpeed_to_Min_or_Max_directly_when_outside)
{
    GameClient client;

    client.setNewSpeed(MIN_SPEED_DEBUG);
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MIN_SPEED_DEBUG);
    client.increaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MIN_SPEED);
    client.setNewSpeed((MIN_SPEED_DEBUG + MIN_SPEED) / 2); // Any speed between
    client.increaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MIN_SPEED);

    client.setNewSpeed(MAX_SPEED_DEBUG);
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MAX_SPEED_DEBUG);
    client.decreaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MAX_SPEED);
    client.setNewSpeed((MAX_SPEED_DEBUG + MAX_SPEED) / 2); // Any speed between
    client.decreaseSpeed();
    BOOST_TEST_REQUIRE(client.getGFLengthReq() == MAX_SPEED);
}
#endif

BOOST_AUTO_TEST_SUITE_END()
