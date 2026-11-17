// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameMessage_Chat.h"
#include "GameMessageInterface.h"
#include "helpers/serializeEnums.h"
#include "s25util/Serializer.h"

void GameMessage_Chat::serialize(Serializer& ser) const
{
    GameMessageWithPlayer::serialize(ser);
    helpers::pushEnum<uint8_t>(ser, destination);
    ser.pushString(text);
}

void GameMessage_Chat::deserialize(Serializer& ser)
{
    GameMessageWithPlayer::deserialize(ser);
    destination = helpers::popEnum<ChatDestination>(ser);
    text = ser.popString();
}

bool GameMessage_Chat::runGame(GameMessageInterface* callback) const
{
    return callback->onGameMessage(*this);
}
