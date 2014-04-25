// $Id: GamePlayerInfo.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

#include "GameProtocol.h"
#include "Socket.h"
#include "GameConsts.h"

class Serializer;

enum PlayerState
{
    PS_FREE = 0,
    PS_RESERVED,
    PS_OCCUPIED,
    PS_LOCKED,
    PS_KI
};

enum AIType
{
    AI_DUMMY = 0,
    AI_JH
};

class GamePlayerInfo
{
    public:
        GamePlayerInfo(const unsigned playerid);
        /// Deserialisierungskonstruktor
        GamePlayerInfo(const unsigned playerid, Serializer* ser);

        virtual ~GamePlayerInfo();

        void clear();

        /// Spielerplatz belegt?
        bool isValid() const { return (ps == PS_RESERVED || ps == PS_OCCUPIED); }

        /// Ist Spieler besiegt?
        bool isDefeated() const { return defeated; }

        /// serialisiert die Daten.
        void serialize(Serializer* ser) const;

        unsigned getPlayerID() const { return playerid; }

    protected:
        /// Wechselt Spieler
        void SwapPlayer(GamePlayerInfo& two);

    protected:
        /// Player-ID
        unsigned playerid;
        /// Besiegt?
        bool defeated;

    public:
        /// Spielertyp (Mensch, KI oder geschlossen..?)
        PlayerState ps;
        /// Wenn KI, was für eine?
        AIType aiType;

        /// Spielername
        std::string name;
        /// ehemaliger Spielername bei einem geladenen Spiel
        std::string origin_name;

        bool is_host;

        Nation nation;
        Team team;
        unsigned char color;

        unsigned ping;
        unsigned int rating;

        int checksum;
        unsigned obj_cnt;
        unsigned obj_id_cnt;
        bool ready;
};

#endif // GAMEPLAYERINFO_H_INCLUDED
