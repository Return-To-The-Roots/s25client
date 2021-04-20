// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameMessage_Chat.h"
#include "GameMessageInterface.h"
#include "helpers/serializeEnums.h"
#include "s25util/Serializer.h"

void GameMessage_Chat::Serialize(Serializer& ser) const
{
    GameMessageWithPlayer::Serialize(ser);
    helpers::pushEnum<uint8_t>(ser, destination);
    ser.PushString(text);
}

void GameMessage_Chat::Deserialize(Serializer& ser)
{
    GameMessageWithPlayer::Deserialize(ser);
    destination = helpers::popEnum<ChatDestination>(ser);
    text = ser.PopString();
}

bool GameMessage_Chat::Run(GameMessageInterface* callback) const
{
    return callback->OnGameMessage(*this);
}
