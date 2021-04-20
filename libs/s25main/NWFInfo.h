// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
