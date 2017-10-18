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
#ifndef AIPLAYER_H_INCLUDED
#define AIPLAYER_H_INCLUDED

#pragma once

#include "AIInterface.h"
#include "GameCommand.h"

class GameWorldBase;
class GamePlayer;
class GlobalGameSettings;

/// Base class for all AI players
class AIPlayer
{
public:
    AIPlayer(const unsigned char playerId, const GameWorldBase& gwb, const AI::Level level)
        : playerId(playerId), player(gwb.GetPlayer(playerId)),  gwb(gwb), ggs(gwb.GetGGS()),level(level), aii(gwb, gcs, playerId)
    {
    }

    virtual ~AIPlayer() {}

    /// Called for every GF
    virtual void RunGF(const unsigned gf, bool gfisnwf) = 0;

    const std::string& GetPlayerName() const { return player.name; }
    unsigned char GetPlayerId() const { return playerId; }

    /// Zugriff auf die GameCommands, um diese abarbeiten zu können
    const std::vector<gc::GameCommandPtr>& GetGameCommands() const { return gcs; }
    /// Markiert die GameCommands als abgearbeitet
    void FetchGameCommands() { gcs.clear(); }

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

#endif //! AIPLAYER_H_INCLUDED
