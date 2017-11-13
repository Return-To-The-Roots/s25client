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
#ifndef GAMEMESSAGE_H_INCLUDED
#define GAMEMESSAGE_H_INCLUDED

#pragma once

#include "libutil/Message.h"
class GameMessageInterface;
class MessageInterface;
class Serializer;

class GameMessage : public Message
{
public:
    /// Spieler-ID, von dem diese Nachricht stammt
    uint8_t player;

    GameMessage(uint16_t id) : Message(id) {} //-V730
    GameMessage(uint16_t id, uint8_t player) : Message(id), player(player) {}

    void Serialize(Serializer& ser) const override;

    void Deserialize(Serializer& ser) override;

    /// Run Methode für GameMessages, wobei PlayerId ggf. schon in der Message festgemacht wurde
    virtual bool Run(GameMessageInterface* callback) = 0;

    bool run(MessageInterface* callback, unsigned id) override
    {
        if(id != 0xFFFFFFFF)
            player = static_cast<uint8_t>(id);
        return Run(checkedCast<GameMessageInterface*>(callback));
    }

    static Message* create_game(unsigned short id);
    Message* create(unsigned short id) const override { return create_game(id); }
};

#endif // GAMEMESSAGE_H_INCLUDED
