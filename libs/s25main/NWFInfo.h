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

#include "network/PlayerGameCommands.h"
#include <queue>
#include <vector>

struct FramesInfo;

struct NWFServerInfo
{
    /// GF at which this should be executed
    unsigned gf;
    /// New GF length in ms
    unsigned newGFLen;
    /// GF at which the next NWF is to be executed
    unsigned nextNWF;
    NWFServerInfo(unsigned gf, unsigned gfLen, unsigned nextNWF) : gf(gf), newGFLen(gfLen), nextNWF(nextNWF) {}
};

struct NWFPlayerInfo
{
    /// Player Id
    unsigned id;
    bool isLagging;
    std::queue<PlayerGameCommands> commands;

    explicit NWFPlayerInfo(unsigned playerId) : id(playerId), isLagging(false) {}
    /// Set isLagging flag to commands.empty()
    void checkLagging();
};

/// Holds information about the NWFs
class NWFInfo
{
    std::queue<NWFServerInfo> serverInfos_;
    std::vector<NWFPlayerInfo> playerInfos_;
    unsigned nextNWF_, cmdDelay_;

public:
    NWFInfo() : nextNWF_(0), cmdDelay_(1) {}
    /// Has to be called on game start with the first server info. Command delay is the number of NWS a command is sent
    /// in advance (>=1)
    void init(unsigned nextNWF, unsigned cmdDelay);

    /// Add an active player with the given id
    void addPlayer(unsigned playerId);
    /// Remove the player (slot closed)
    void removePlayer(unsigned playerId);
    /// Add cmds for this player
    bool addPlayerCmds(unsigned playerId, const PlayerGameCommands& cmds);
    /// Add the server info
    bool addServerInfo(const NWFServerInfo& info);
    /// Checks if we can execute a NWF and set the isLagging state
    bool isReady();
    /// Get the player. Throws if player id is invalid
    const NWFPlayerInfo& getPlayerInfo(unsigned playerId) const;
    /// Get the current player cmds. Throws if player id is invalid or the player has no cmds (isReady == false)
    const PlayerGameCommands& getPlayerCmds(unsigned playerId) const;
    /// Get the current server info. Throws if none (isReady == false)
    const NWFServerInfo& getServerInfo() const;
    /// Execute the rest of the NWF by filling the info struct with the new data and pop the handled cmds
    void execute(FramesInfo& info);
    const std::vector<NWFPlayerInfo>& getPlayerInfos() const { return playerInfos_; }
    /// Which GF to execute the next NWF
    unsigned getNextNWF() const { return nextNWF_; }
    /// Return the nextNWF from the last serverInfo entry (must exist)
    unsigned getLastNWF() const;
    /// Number of NWFs a command is sent in advance (>= 1)
    unsigned getCmdDelay() const { return cmdDelay_; }
};
