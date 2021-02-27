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

#pragma once

#include "GameMessage.h"
#include "GameProtocol.h"
#include "gameTypes/ChatDestination.h"
#include <string>

class GameMessage_Chat : public GameMessageWithPlayer
{
public:
    ChatDestination destination;
    std::string text;

    GameMessage_Chat() : GameMessageWithPlayer(NMS_CHAT) {} //-V730
    GameMessage_Chat(uint8_t player, const ChatDestination destination, std::string text)
        : GameMessageWithPlayer(NMS_CHAT, player), destination(destination), text(std::move(text))
    {}

    void Serialize(Serializer& ser) const override;
    void Deserialize(Serializer& ser) override;
    bool Run(GameMessageInterface* callback) const override;
};
