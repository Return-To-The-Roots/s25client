// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

void GameMessage_GameCommand::serialize(Serializer& ser) const
{
    GameMessageWithPlayer::serialize(ser);
    cmds.serialize(ser);
}

void GameMessage_GameCommand::deserialize(Serializer& ser)
{
    GameMessageWithPlayer::deserialize(ser);
    gc::Deserializer deser(ser);
    cmds.deserialize(deser);
}

bool GameMessage_GameCommand::runGame(GameMessageInterface* callback) const
{
    return callback->onGameMessage(*this);
}
