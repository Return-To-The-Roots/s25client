// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "JoinPlayerInfo.h"
#include "network/GameMessages.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameTypes/PlayerState.h"
#include "rttr/test/random.hpp"
#include <boost/test/unit_test.hpp>

bool operator==(const JoinPlayerInfo& lhs, const JoinPlayerInfo& rhs)
{
    const auto getMembers = [](const JoinPlayerInfo& info) {
        return std::tie(info.ps, info.aiInfo, info.name, info.name, info.color, info.team, info.isHost, info.ping,
                        info.originName, info.isReady);
    };
    return getMembers(lhs) == getMembers(rhs);
}

// LCOV_EXCL_START
BOOST_TEST_DONT_PRINT_LOG_VALUE(AI::Info)
BOOST_TEST_DONT_PRINT_LOG_VALUE(JoinPlayerInfo)
BOOST_TEST_DONT_PRINT_LOG_VALUE(std::unique_ptr<GameMessage>)

template<typename T>
static std::enable_if_t<std::is_enum<T>::value, std::ostream&> operator<<(std::ostream& os, T enumVal)
{
    return os << rttr::enum_cast(enumVal);
}
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(GameMessages)

template<class TMsg>
auto serializeDeserializeMessage(const TMsg& msgIn)
{
    static Serializer ser;
    ser.Clear();
    msgIn.Serialize(ser);
    Message* msgOut(GameMessage::create_game(msgIn.getId()));
    BOOST_TEST_REQUIRE(msgOut);
    std::unique_ptr<TMsg> msgResult(dynamic_cast<TMsg*>(msgOut));
    BOOST_TEST_REQUIRE(!!msgResult);
    Serializer serOut(ser.GetData(), ser.GetLength());
    msgOut->Deserialize(serOut);
    BOOST_TEST(serOut.GetBytesLeft() == 0u);
    return msgResult;
}

BOOST_AUTO_TEST_CASE(Serialization)
{
    using rttr::test::randomBool;
    using rttr::test::randomEnum;
    using rttr::test::randomValue;
    using rttr::test::randString;

    {
        const GameMessage_Ping msgIn(randomValue<uint8_t>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->player == msgIn.player);
    }
    {
        const auto msgOut = serializeDeserializeMessage(GameMessage_Pong());
        BOOST_TEST(msgOut); // Only exist
    }
    {
        const GameMessage_Server_Type msgIn(randomEnum<ServerType>(), randString());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->type == msgIn.type);
        BOOST_TEST(msgOut->revision == msgIn.revision);
    }
    {
        const GameMessage_Server_TypeOK msgIn(randomEnum<GameMessage_Server_TypeOK::StatusCode>(), randString());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->err_code == msgIn.err_code);
        BOOST_TEST(msgOut->version == msgIn.version);
    }
    {
        const GameMessage_Server_Password msgIn(randString());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->password == msgIn.password);
    }
    {
        const GameMessage_Server_Name msgIn(randString());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->name == msgIn.name);
    }
    {
        const GameMessage_Server_Start msgIn(randomValue<unsigned>(), randomValue<unsigned>(), randomValue<unsigned>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->random_init == msgIn.random_init);
        BOOST_TEST(msgOut->firstNwf == msgIn.firstNwf);
        BOOST_TEST(msgOut->cmdDelay == msgIn.cmdDelay);
    }
    {
        const GameMessage_Countdown msgIn(randomValue<unsigned>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->countdown == msgIn.countdown);
    }
    {
        const GameMessage_CancelCountdown msgIn(randomBool());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->error == msgIn.error);
    }
    {
        const GameMessage_Server_Async msgIn(std::vector<unsigned>(randomValue(0, 3), randomValue<unsigned>()));
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->checksums == msgIn.checksums, boost::test_tools::per_element());
    }
    {
        const GameMessage_Player_Id msgIn(randomValue<uint8_t>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->player == msgIn.player);
    }
    {
        const GameMessage_Player_Name msgIn(randomValue<uint8_t>(), randString());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->player == msgIn.player);
        BOOST_TEST(msgOut->playername == msgIn.playername);
    }
    {
        std::vector<JoinPlayerInfo> playerInfos(randomValue(1, 8));
        for(JoinPlayerInfo& player : playerInfos)
        {
            player.ps = randomEnum<PlayerState>();
            player.aiInfo.type = randomEnum<AI::Type>();
            player.aiInfo.level = randomEnum<AI::Level>();
            player.name = randString();
            player.nation = randomEnum<Nation>();
            player.color = randomValue<unsigned>();
            player.team = randomEnum<Team>();
            player.isHost = randomBool();
            player.ping = randomValue<unsigned>();
            player.originName = randString();
            player.isReady = randomBool();
        }
        const GameMessage_Player_List msgIn(playerInfos);
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->playerInfos == msgIn.playerInfos, boost::test_tools::per_element());
    }
    {
        const GameMessage_Player_State msgIn(randomValue<uint8_t>(), randomEnum<PlayerState>(),
                                             {randomEnum<AI::Type>(), randomEnum<AI::Level>()});
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->player == msgIn.player);
        BOOST_TEST(msgOut->aiInfo == msgIn.aiInfo);
    }
    {
        const GameMessage_Player_Nation msgIn(randomValue<uint8_t>(), randomEnum<Nation>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->player == msgIn.player);
        BOOST_TEST(msgOut->nation == msgIn.nation);
    }
    {
        const GameMessage_Player_Team msgIn(randomValue<uint8_t>(), randomEnum<Team>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->player == msgIn.player);
        BOOST_TEST(msgOut->team == msgIn.team);
    }
    {
        const GameMessage_Player_Color msgIn(randomValue<uint8_t>(), randomValue<uint32_t>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->player == msgIn.player);
        BOOST_TEST(msgOut->color == msgIn.color);
    }
    {
        const GameMessage_Player_Kicked msgIn(randomValue<uint8_t>(), randomEnum<KickReason>(),
                                              randomValue<uint32_t>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->player == msgIn.player);
        BOOST_TEST(msgOut->cause == msgIn.cause);
        BOOST_TEST(msgOut->param == msgIn.param);
    }
    {
        const GameMessage_Player_Ping msgIn(randomValue<uint8_t>(), randomValue<uint16_t>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->player == msgIn.player);
        BOOST_TEST(msgOut->ping == msgIn.ping);
    }
    {
        const GameMessage_Player_New msgIn(randomValue<uint8_t>(), randString());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->player == msgIn.player);
        BOOST_TEST(msgOut->name == msgIn.name);
    }
    {
        const GameMessage_Player_Ready msgIn(randomValue<uint8_t>(), randomBool());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->player == msgIn.player);
        BOOST_TEST(msgOut->ready == msgIn.ready);
    }
    {
        const GameMessage_Player_Swap msgIn(randomValue<uint8_t>(), randomValue<uint8_t>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->player == msgIn.player);
        BOOST_TEST(msgOut->player2 == msgIn.player2);
    }
    {
        const GameMessage_Player_SwapConfirm msgIn(randomValue<uint8_t>(), randomValue<uint8_t>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->player == msgIn.player);
        BOOST_TEST(msgOut->player2 == msgIn.player2);
    }
    {
        const GameMessage_Map_Info msgIn(randString(), randomEnum<MapType>(), randomValue<unsigned>(),
                                         randomValue<unsigned>(), randomValue<unsigned>(), randomValue<unsigned>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->filename == msgIn.filename);
        BOOST_TEST(msgOut->mt == msgIn.mt);
        BOOST_TEST(msgOut->mapLen == msgIn.mapLen);
        BOOST_TEST(msgOut->mapCompressedLen == msgIn.mapCompressedLen);
        BOOST_TEST(msgOut->luaLen == msgIn.luaLen);
        BOOST_TEST(msgOut->luaCompressedLen == msgIn.luaCompressedLen);
    }
    {
        const GameMessage_MapRequest msgIn(randomBool());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->requestInfo == msgIn.requestInfo);
    }
    {
        std::vector<char> data(randomValue(1, 20));
        for(auto& c : data)
            c = randomValue<char>();
        const GameMessage_Map_Data msgIn(randomBool(), randomValue<uint32_t>(), data.data(), data.size());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->isMapData == msgIn.isMapData);
        BOOST_TEST(msgOut->offset == msgIn.offset);
        BOOST_TEST(msgOut->data == msgIn.data);
    }
    {
        const GameMessage_Map_Checksum msgIn(randomValue<unsigned>(), randomValue<unsigned>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->mapChecksum == msgIn.mapChecksum);
        BOOST_TEST(msgOut->luaChecksum == msgIn.luaChecksum);
    }
    {
        const GameMessage_Map_ChecksumOK msgIn(randomBool(), randomBool());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->correct == msgIn.correct);
        BOOST_TEST(msgOut->retryAllowed == msgIn.retryAllowed);
    }
    {
        GameMessage_GGSChange msgIn(GlobalGameSettings{});
        msgIn.ggs.exploration = randomEnum<Exploration>(); // Just set (and test) any, not the whole struct
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->ggs.exploration == msgIn.ggs.exploration);
    }
    {
        const GameMessage_Speed msgIn(randomValue<unsigned>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->gf_length == msgIn.gf_length);
    }
    {
        const GameMessage_Server_NWFDone msgIn(randomValue<unsigned>(), randomValue<unsigned>(),
                                               randomValue<unsigned>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->gf == msgIn.gf);
        BOOST_TEST(msgOut->gf_length == msgIn.gf_length);
        BOOST_TEST(msgOut->nextNWF == msgIn.nextNWF);
    }
    {
        const GameMessage_Pause msgIn(randomBool());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->paused == msgIn.paused);
    }
    {
        const GameMessage_SkipToGF msgIn(randomValue<unsigned>());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->targetGF == msgIn.targetGF);
    }
    {
        const auto msgOut = serializeDeserializeMessage(GameMessage_GetAsyncLog());
        BOOST_TEST(msgOut); // Just exist
    }
    {
        const GameMessage_AsyncLog msgIn(randString());
        const auto msgOut = serializeDeserializeMessage(msgIn);
        BOOST_TEST(msgOut->addData == msgIn.addData);
        BOOST_TEST(!msgOut->last);
        BOOST_TEST(msgOut->entries.empty());

        std::vector<RandomEntry> async_log(randomValue(1, 5));
        for(auto& entry : async_log)
        {
            entry.counter = randomValue<unsigned>();
            entry.maxExcl = randomValue<int>();
            entry.srcName = randString();
            entry.srcLine = randomValue<unsigned>();
            entry.objId = randomValue<unsigned>();
        }
        const GameMessage_AsyncLog msgIn2(std::move(async_log), randomBool());
        const auto msgOut2 = serializeDeserializeMessage(msgIn2);
        BOOST_TEST(msgOut2->addData.empty());
        BOOST_TEST(msgOut2->last == msgIn2.last);
        BOOST_TEST_REQUIRE(msgOut2->entries.size() == msgIn2.entries.size());
        for(unsigned i = 0; i < msgIn2.entries.size(); i++)
        {
            BOOST_TEST(msgOut2->entries[i].counter == msgIn2.entries[i].counter);
            BOOST_TEST(msgOut2->entries[i].maxExcl == msgIn2.entries[i].maxExcl);
            BOOST_TEST(msgOut2->entries[i].srcName == msgIn2.entries[i].srcName);
            BOOST_TEST(msgOut2->entries[i].srcLine == msgIn2.entries[i].srcLine);
            BOOST_TEST(msgOut2->entries[i].objId == msgIn2.entries[i].objId);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
