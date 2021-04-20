// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameMessage_GameCommand.h"
#include "GameMessageInterface.h"
#include "GameProtocol.h"

//////////////////////////////////////////////////////////////////////////

GameMessage_GameCommand::GameMessage_GameCommand() : GameMessageWithPlayer(NMS_GAMECOMMANDS) {}

GameMessage_GameCommand::GameMessage_GameCommand(uint8_t player, const AsyncChecksum& checksum,
                                                 const std::vector<gc::GameCommandPtr>& gcs)
    : GameMessageWithPlayer(NMS_GAMECOMMANDS, player), cmds(checksum, gcs)
{}

void GameMessage_GameCommand::Serialize(Serializer& ser) const
{
    GameMessageWithPlayer::Serialize(ser);
    cmds.Serialize(ser);
}

void GameMessage_GameCommand::Deserialize(Serializer& ser)
{
    GameMessageWithPlayer::Deserialize(ser);
    cmds.Deserialize(ser);
}

bool GameMessage_GameCommand::Run(GameMessageInterface* callback) const
{
    return callback->OnGameMessage(*this);
}
