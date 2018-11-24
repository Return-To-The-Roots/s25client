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

#include "AsyncChecksum.h"
#include "NetworkPlayer.h"
#include "Timer.h"
#include <queue>

class Serializer;

/// Player connected to the server
class GameServerPlayer : public NetworkPlayer
{
public:
    GameServerPlayer(unsigned id, const Socket& socket);
    ~GameServerPlayer();

    void setConnected();
    bool isConnected() const { return !connectTimer.isRunning(); }
    /// Get seconds till the player gets kicked due to lag
    unsigned getLagTimeOut() const;
    /// Ping the player if required
    void doPing();
    /// Called when a ping response was received. Return the ping in ms
    unsigned calcPingTime();
    /// Check if the player timed out.
    bool hasTimedOut() const;

    /// Set player is lagging
    void setLagging();
    /// Set player not lagging (anymore)
    void setNotLagging();

private:
    /// Running if the player is connecting (reserved slot)
    Timer connectTimer;
    /// Are we waiting for a ping reply
    bool isPinging;
    /// Timer for the current sent ping command or the last received ping reply
    Timer pingTimer;
    /// Timer started when the player started lagging
    Timer lagTimer;

public:
    bool mapDataSent;
    /// These swaps are yet to be confirmed by the client
    struct PendingSwap
    {
        unsigned playerId1, playerId2;
    };
    std::vector<PendingSwap> pendingSwaps;
};

#endif // GAMESERVERPLAYER_H_INCLUDED
