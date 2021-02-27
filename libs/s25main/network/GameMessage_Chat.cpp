// Copyright (c) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
