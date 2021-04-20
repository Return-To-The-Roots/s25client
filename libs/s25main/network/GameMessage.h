// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
