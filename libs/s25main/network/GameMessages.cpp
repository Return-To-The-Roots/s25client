// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameMessages.h"
#include "JoinPlayerInfo.h"
#include "enum_cast.hpp"
#include "helpers/serializeContainers.h"

GameMessage_Player_List::GameMessage_Player_List() : GameMessage(NMS_PLAYER_LIST) {}

GameMessage_Player_List::GameMessage_Player_List(const std::vector<JoinPlayerInfo>& playerInfos)
    : GameMessage(NMS_PLAYER_LIST), playerInfos(playerInfos)
{
    LOG.writeToFile(">>> NMS_PLAYER_LIST(%u)\n") % playerInfos.size();
}

GameMessage_Player_List::~GameMessage_Player_List() = default;

void GameMessage_Player_List::Serialize(Serializer& ser) const
{
    GameMessage::Serialize(ser);
    ser.PushUnsignedInt(playerInfos.size());
    for(const auto& playerInfo : playerInfos)
        playerInfo.Serialize(ser);
}

void GameMessage_Player_List::Deserialize(Serializer& ser)
{
    GameMessage::Deserialize(ser);
    unsigned numPlayers = ser.PopUnsignedInt();
    playerInfos.clear();
    playerInfos.reserve(numPlayers);
    for(unsigned i = 0; i < numPlayers; ++i)
        playerInfos.push_back(JoinPlayerInfo(ser));
}

bool GameMessage_Player_List::Run(GameMessageInterface* callback) const
{
    LOG.writeToFile("<<< NMS_PLAYER_LIST(%d)\n") % playerInfos.size();
    for(unsigned i = 0; i < playerInfos.size(); ++i)
    {
        const JoinPlayerInfo& playerInfo = playerInfos[i];
        LOG.writeToFile("    %d: %s %d %d %d %d %d %s\n") % i % playerInfo.name % rttr::enum_cast(playerInfo.ps)
          % playerInfo.ping % rttr::enum_cast(playerInfo.nation) % playerInfo.color % rttr::enum_cast(playerInfo.team)
          % (playerInfo.isReady ? "true" : "false");
    }
    return callback->OnGameMessage(*this);
}

void GameMessage_Server_Async::Serialize(Serializer& ser) const
{
    GameMessage::Serialize(ser);
    helpers::pushContainer(ser, checksums);
}

void GameMessage_Server_Async::Deserialize(Serializer& ser)
{
    GameMessage::Deserialize(ser);
    helpers::popContainer(ser, checksums);
}
