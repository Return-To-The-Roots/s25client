// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "AIInterface.h"
#include "GameCommand.h"
#include "gameTypes/ChatDestination.h"

class GameWorldBase;
class GamePlayer;
class GlobalGameSettings;

/// Base class for all AI players
class AIPlayer
{
public:
    AIPlayer(unsigned char playerId, const GameWorldBase& gwb, const AI::Level level)
        : playerId(playerId), player(gwb.GetPlayer(playerId)), gwb(gwb), ggs(gwb.GetGGS()), level(level),
          aii(gwb, gcs, playerId)
    {}

    virtual ~AIPlayer() = default;

    /// Called for every GF
    virtual void RunGF(unsigned gf, bool gfisnwf) = 0;
    virtual void OnChatMessage(unsigned sendPlayerId, ChatDestination, const std::string& msg) = 0;

    const std::string& GetPlayerName() const { return player.name; }
    unsigned char GetPlayerId() const { return playerId; }
    AI::Level GetLevel() const { return level; }

    /// Get the game commands and mark them as processed
    std::vector<gc::GameCommandPtr> FetchGameCommands()
    {
        std::vector<gc::GameCommandPtr> tmp;
        std::swap(tmp, gcs);
        return tmp;
    }

    // access to ais CommandFactory
    const AIInterface& getAIInterface() const { return aii; }
    AIInterface& getAIInterface() { return aii; }

    virtual void saveStats(unsigned /*gf*/) const {}

    /// Own player ID that the AI should know, e.g. when exploring the map
    const unsigned char playerId;
    /// Reference to the own GameClientPlayer, i.e. the economy, to gather relevant information from it
    const GamePlayer& player;
    /// Reference to the game world to gather relevant information from it
    const GameWorldBase& gwb;
    /// Reference to global game settings, as they can also influence further decisions
    /// (for example victory conditions, FoW, etc.)
    const GlobalGameSettings& ggs;

protected:
    /// Queue of game commands that still need to be processed
    std::vector<gc::GameCommandPtr> gcs;
    /// AI difficulty level
    const AI::Level level;
    /// Abstracted interface that forwards commands
    AIInterface aii;
};
