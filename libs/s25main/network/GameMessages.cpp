// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "GameMessages.h"
#include "JoinPlayerInfo.h"

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
        LOG.writeToFile("    %d: %s %d %d %d %d %d %s\n") % i % playerInfo.name % playerInfo.ps % playerInfo.ping % playerInfo.nation
          % playerInfo.color % playerInfo.team % (playerInfo.isReady ? "true" : "false");
    }
    return callback->OnGameMessage(*this);
}
