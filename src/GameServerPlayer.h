// $Id: GameServerPlayer.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef GAMESERVERPLAYER_H_INCLUDED
#define GAMESERVERPLAYER_H_INCLUDED

#pragma once

#include "GamePlayerInfo.h"
//#include "GameMessages.h"
#include "MessageQueue.h"

#include <list>

class GameMessage_GameCommand;
class Serializer;

// GamePlayerInfo für die PlayerSlots des Servers
class GameServerPlayer : public GamePlayerInfo
{
    public:
        GameServerPlayer(const unsigned playerid);
        GameServerPlayer(const unsigned playerid, Serializer* ser);
        ~GameServerPlayer();

        /// Gibt Sekunden bis zum TimeOut (Rausschmiss) zurück
        unsigned GetTimeOut() const;

        void doPing();
        void doTimeout();
        void reserve(Socket* sock, unsigned char id);
        void clear();

        /// Tauscht Spieler
        void SwapPlayer(GameServerPlayer& two);

        /// Spieler laggt
        void Lagging();
        /// Spieler laggt nicht (mehr)
        void NotLagging();

    private:

        unsigned int connecttime;
        /// Zeitpunkt, ab dem kein Kommando mehr vom Spieler kommt
        unser_time_t last_command_timeout;

    public:
        Socket so;
        bool pinging;

        MessageQueue send_queue;
        MessageQueue recv_queue;

        std::list<GameMessage_GameCommand> gc_queue;

        unsigned int lastping;

        unsigned int temp_ul;
        unsigned int temp_ui;
};


#endif // GAMESERVERPLAYER_H_INCLUDED
