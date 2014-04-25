// $Id: Messenger.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef MESSENGER_H_INCLUDED
#define MESSENGER_H_INCLUDED

#pragma once

#include "GameProtocol.h"
#include <list>

class Messenger
{
        struct Msg
        {
            std::string author;
            unsigned color_author;
            ChatDestination cd;
            std::string msg;
            unsigned color_msg;
            unsigned starttime;
            unsigned short width;
        };

        std::list<Msg> messages;

    public:
        ~Messenger();

        void Draw();
        void AddMessage(const std::string& author, const unsigned color_author, const ChatDestination cd, const std::string& msg, const unsigned color_msg = COLOR_YELLOW);
};

#endif // !MESSENGER_H_INCLUDED
