// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "Message.h"

class GameMessage : public Message
{
    public:
        /// Spieler-ID, von dem diese Nachricht stammt
        unsigned char player;

        GameMessage(const unsigned short id) : Message(id) {}
        /// Konstruktor von @p GameMessage.
        GameMessage(const unsigned short id, const unsigned char player): Message(id), player(player){}

        void Serialize(Serializer& ser) const override
        {
            Message::Serialize(ser);
            ser.PushUnsignedChar(player);
        }

        void Deserialize(Serializer& ser) override
        {
            Message::Deserialize(ser);
            player = ser.PopUnsignedChar();
        }

        /// Run Methode für GameMessages, wobei PlayerID ggf. schon in der Message festgemacht wurde
        virtual void Run(MessageInterface* callback) = 0;

        virtual void run(MessageInterface* callback, unsigned int id)
        {
            if(id != 0xFFFFFFFF)
                player = static_cast<unsigned char>(id);
            Run(callback);
        }

        static Message* create_game(unsigned short id);
        virtual Message* create(unsigned short id) const { return create_game(id); }
};

#endif // GAMEMESSAGE_H_INCLUDED
