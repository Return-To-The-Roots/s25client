// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameMessages.h"
#include "JoinPlayerInfo.h"
#include "enum_cast.hpp"
#include "helpers/serializeContainers.h"
#include "helpers/serializeEnums.h"

GameMessage_Player_List::GameMessage_Player_List() : GameMessage(NMS_PLAYER_LIST) {}

GameMessage_Player_List::GameMessage_Player_List(const std::vector<JoinPlayerInfo>& playerInfos)
    : GameMessage(NMS_PLAYER_LIST), playerInfos(playerInfos)
{
    LOG.writeToFile(">>> NMS_PLAYER_LIST(%u)\n") % playerInfos.size();
}

GameMessage_Player_List::~GameMessage_Player_List() = default;

void GameMessage_Player_List::serialize(Serializer& ser) const
{
    GameMessage::serialize(ser);
    ser.pushUnsignedInt(playerInfos.size());
    for(const auto& playerInfo : playerInfos)
        playerInfo.serialize(ser);
}

void GameMessage_Player_List::deserialize(Serializer& ser)
{
    GameMessage::deserialize(ser);
    unsigned numPlayers = ser.popUnsignedInt();
    playerInfos.clear();
    playerInfos.reserve(numPlayers);
    for(unsigned i = 0; i < numPlayers; ++i)
        playerInfos.push_back(JoinPlayerInfo(ser));
}

bool GameMessage_Player_List::runGame(GameMessageInterface* callback) const
{
    LOG.writeToFile("<<< NMS_PLAYER_LIST(%d)\n") % playerInfos.size();
    for(unsigned i = 0; i < playerInfos.size(); ++i)
    {
        const JoinPlayerInfo& playerInfo = playerInfos[i];
        LOG.writeToFile("    %d: %s %d %d %d %d %d %s\n") % i % playerInfo.name % rttr::enumCast(playerInfo.ps)
          % playerInfo.ping % rttr::enumCast(playerInfo.nation) % playerInfo.color % rttr::enumCast(playerInfo.team)
          % (playerInfo.isReady ? "true" : "false");
    }
    return callback->onGameMessage(*this);
}

void GameMessage_Server_Async::serialize(Serializer& ser) const
{
    GameMessage::serialize(ser);
    helpers::pushContainer(ser, checksums);
}

void GameMessage_Server_Async::deserialize(Serializer& ser)
{
    GameMessage::deserialize(ser);
    helpers::popContainer(ser, checksums);
}

void GameMessage_Server_TypeOK::serialize(Serializer& ser) const
{
    GameMessage::serialize(ser);
    helpers::pushEnum<uint32_t>(ser, err_code);
    ser.pushString(version);
}

void GameMessage_Server_TypeOK::deserialize(Serializer& ser)
{
    GameMessage::deserialize(ser);
    err_code = helpers::popEnum<StatusCode>(ser);
    version = ser.popString();
}
