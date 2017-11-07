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
#ifndef GAMESERVERPLAYER_H_INCLUDED
#define GAMESERVERPLAYER_H_INCLUDED

#pragma once

#include "JoinPlayerInfo.h"
#include "libutil/MessageQueue.h"
#include "libutil/MyTime.h"
#include "libutil/Socket.h"
#include <vector>

class GameMessage_GameCommand;
class Serializer;

// GamePlayerInfo für die PlayerSlots des Servers
class GameServerPlayer : public JoinPlayerInfo
{
public:
    GameServerPlayer();
    ~GameServerPlayer();

    /// Gibt Sekunden bis zum TimeOut (Rausschmiss) zurück
    unsigned GetTimeOut() const;

    void doPing();
    void checkConnectTimeout();
    void reserve(const Socket& sock);
    void CloseConnections();

    /// Spieler laggt
    void Lagging();
    /// Spieler laggt nicht (mehr)
    void NotLagging();

private:
    unsigned connecttime;
    /// Zeitpunkt, ab dem kein Kommando mehr vom Spieler kommt
    s25util::time64_t last_command_timeout;

public:
    Socket so;
    bool pinging;

    MessageQueue send_queue;
    MessageQueue recv_queue;

    std::vector<GameMessage_GameCommand> gc_queue;

    unsigned lastping;
};

#endif // GAMESERVERPLAYER_H_INCLUDED
