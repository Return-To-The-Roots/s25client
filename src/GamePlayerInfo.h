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

#ifndef GAMEPLAYERINFO_H_INCLUDED
#define GAMEPLAYERINFO_H_INCLUDED

#include "gameTypes/AIInfo.h"
#include "gameTypes/PlayerState.h"
#include "gameData/NationConsts.h"
#include "gameData/PlayerConsts.h"

class Serializer;

class GamePlayerInfo
{
    public:
        GamePlayerInfo(const unsigned playerid);
        GamePlayerInfo(const unsigned playerid, Serializer& ser);

        virtual ~GamePlayerInfo();

        void clear();

        /// Slot used by a human player (has socket etc)
        bool isHuman() const { return (ps == PS_RESERVED || ps == PS_OCCUPIED); }
        /// Slot filled (Used by human or AI, but excludes currently connecting humans)
        bool isUsed() const { return (ps == PS_KI || ps == PS_OCCUPIED); }

        /// Ist Spieler besiegt?
        bool isDefeated() const { return defeated; }

        /// serialisiert die Daten.
        void serialize(Serializer& ser) const;

        unsigned getPlayerID() const { return playerid; }

        /// Wechselt Spieler
        void SwapInfo(GamePlayerInfo& two);
        /// Returns index of color in PLAYER_COLORS array or -1 if not found
        int GetColorIdx() const;
        static int GetColorIdx(unsigned color);

    protected:
        /// Player-ID
        unsigned playerid;
        /// Besiegt?
        bool defeated;

    public:
        /// Spielertyp (Mensch, KI oder geschlossen..?)
        PlayerState ps;
        /// Wenn KI, was für eine?
        AI::Info aiInfo;

        /// Spielername
        std::string name;
        /// ehemaliger Spielername bei einem geladenen Spiel
        std::string origin_name;

        bool is_host;

        Nation nation;
        Team team;
        /// Actual color (ARGB)
        unsigned color;

        unsigned ping;
        unsigned int rating;

        bool ready;
};

#endif // GAMEPLAYERINFO_H_INCLUDED
