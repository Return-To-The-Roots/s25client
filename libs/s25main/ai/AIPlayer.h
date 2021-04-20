// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

    /// Eigene PlayerId, die der KI-Spieler wissen sollte, z.B. wenn er die Karte untersucht
    const unsigned char playerId;
    /// Verweis auf den eigenen GameClientPlayer, d.h. die Wirtschaft, um daraus entsprechend Informationen zu gewinnen
    const GamePlayer& player;
    /// Verweis auf die Spielwelt, um entsprechend Informationen daraus zu erhalten
    const GameWorldBase& gwb;
    /// Verweis auf die Globalen Spieleinstellungen, da diese auch die weiteren Entscheidungen beeinflussen können
    /// (beispielsweise Siegesbedingungen, FOW usw.)
    const GlobalGameSettings& ggs;

protected:
    /// Queue der GameCommands, die noch bearbeitet werden müssen
    std::vector<gc::GameCommandPtr> gcs;
    /// Stärke der KI
    const AI::Level level;
    /// Abstrahiertes Interfaces, leitet Befehle weiter an
    AIInterface aii;
};
