// Copyright (c) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
