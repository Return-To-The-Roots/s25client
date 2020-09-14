// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "s25util/Message.h"

class GameMessageInterface;
class MessageInterface;
class Serializer;

class GameMessage : public Message
{
public:
    /// player ID who sent this message. Only meaningful for server
    uint8_t senderPlayerID = 0xFF;

    GameMessage(uint16_t id) : Message(id) {}

    /// Run Methode für GameMessages, wobei PlayerId ggf. schon in der Message festgemacht wurde
    virtual bool Run(GameMessageInterface* callback) const = 0;

    bool run(MessageInterface* callback, unsigned senderPlayerID) override;

    static Message* create_game(unsigned short id);
    Message* create(unsigned short id) const override { return create_game(id); }
};

/// Game message that optionally has a player ID
class GameMessageWithPlayer : public GameMessage
{
public:
    static constexpr uint8_t NO_PLAYER_ID = 0xFF;
    /// Player set in the message
    uint8_t player;

    GameMessageWithPlayer(uint16_t id, uint8_t player = NO_PLAYER_ID) : GameMessage(id), player(player) {}

    void Serialize(Serializer& ser) const override;
    void Deserialize(Serializer& ser) override;
};
