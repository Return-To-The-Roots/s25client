// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef AIBASE_H_INCLUDED
#define AIBASE_H_INCLUDED

#pragma once

#include "AIEventManager.h"
#include "AIInterface.h"
#include "GameCommand.h"

class GameWorldBase;
class GameClientPlayer;
class GlobalGameSettings;
class GameClientPlayerList;



/// Basisklasse für sämtliche KI-Spieler
class AIBase
{
    protected:
        /// Eigene PlayerID, die der KI-Spieler wissen sollte, z.B. wenn er die Karte untersucht
        const unsigned char playerid;
        /// Verweis auf die Spielwelt, um entsprechend Informationen daraus zu erhalten
        const GameWorldBase& gwb;
        /// Verweis auf den eigenen GameClientPlayer, d.h. die Wirtschaft, um daraus entsprechend Informationen zu gewinnen
        const GameClientPlayer& player;
        /// Verweis auf etwaige andere Spieler, bspw. um deren Bündnisse zu überprüfen etc.
        const GameClientPlayerList& players;
        /// Queue der GameCommands, die noch bearbeitet werden müssen
        std::vector<gc::GameCommandPtr> gcs;
        /// Stärke der KI
        const AI::Level level;
        /// Abstrahiertes Interfaces, leitet Befehle weiter an
        AIInterface* aii;

    public:

        AIBase(const unsigned char playerid, const GameWorldBase& gwb, const GameClientPlayer& player,
               const GameClientPlayerList& players, const GlobalGameSettings& ggs, const AI::Level level)
            : playerid(playerid), gwb(gwb), player(player), players(players), level(level), aii(new AIInterface(gwb, player, players, gcs, playerid)), ggs(ggs) {}

        virtual ~AIBase() {}

        /// Wird jeden GF aufgerufen und die KI kann hier entsprechende Handlungen vollziehen
        virtual void RunGF(const unsigned gf, bool gfisnwf) = 0;

        /// Verweis auf die Globalen Spieleinstellungen, da diese auch die weiteren Entscheidungen beeinflussen können
        /// (beispielsweise Siegesbedingungen, FOW usw.)
        const GlobalGameSettings& ggs;

        /// Zugriff auf die GameCommands, um diese abarbeiten zu können
        const std::vector<gc::GameCommandPtr>& GetGameCommands() const { return gcs; }
        /// Markiert die GameCommands als abgearbeitet
        void FetchGameCommands() { gcs.clear(); }

        virtual void SendAIEvent(AIEvent::Base* ev) { delete ev; }
};

#endif //!AIBASE_H_INCLUDED
