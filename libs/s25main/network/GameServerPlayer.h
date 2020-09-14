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

#include "NetworkPlayer.h"
#include "Timer.h"
#include "helpers/SmoothedValue.hpp"
#include <variant.h>

/// Player connected to the server
class GameServerPlayer : public NetworkPlayer
{
    struct JustConnectedState
    {
        Timer timer;
    };
    struct MapSendingState
    {
        Timer timer;
        std::chrono::seconds estimatedSendTime;
    };
    struct ActiveState
    {
        /// Timer for the current sent ping command or the last received ping reply
        Timer pingTimer;
        /// Timer started when the player started lagging
        Timer lagTimer;
        helpers::SmoothedValue<unsigned> ping;
        /// These swaps are yet to be confirmed by the client
        std::vector<std::pair<unsigned, unsigned>> pendingSwaps;
        /// Are we waiting for a ping reply
        bool isPinging;
        ActiveState(unsigned maxSmoothValues) : ping(maxSmoothValues), isPinging(false) {}
    };

public:
    GameServerPlayer(unsigned id, const Socket& socket);
    ~GameServerPlayer();

    void setMapSending(std::chrono::seconds estimatedSendTime);
    void setActive();
    bool isMapSending() const { return holds_alternative<MapSendingState>(state_); }
    bool isActive() const { return holds_alternative<ActiveState>(state_); }

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

    auto& getPendingSwaps() { return boost::get<ActiveState>(state_).pendingSwaps; }

private:
    boost::variant<JustConnectedState, MapSendingState, ActiveState> state_;
};
