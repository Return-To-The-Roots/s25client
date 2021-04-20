// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
