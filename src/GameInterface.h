﻿// $Id: GameInterface.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef GAMEINTERFACE_H_
#define GAMEINTERFACE_H_

#include "gameTypes/MapTypes.h"

/// Interface, welches vom Spiel angesprocehn werden kann, um beispielsweise GUI wichtige Nachrichten
/// zu übermiteln
class GameInterface
{
    public:
        virtual ~GameInterface() {}

        /// Ein Spieler hat verloren
        virtual void GI_PlayerDefeated(const unsigned player_id) = 0;
        /// Es wurde etwas Minimap entscheidendes geändert --> Minimap updaten
        virtual void GI_UpdateMinimap(const MapPoint pt) = 0;
        /// Flagge wurde zerstört
        virtual void GI_FlagDestroyed(const MapPoint pt) = 0;
        /// Bündnisvertrag wurde abgeschlossen oder abgebrochen --> Minimap updaten
        virtual void GI_TreatyOfAllianceChanged() = 0;

        virtual void GI_Winner(const unsigned player_id) = 0;
        virtual void GI_TeamWinner(const unsigned player_id) = 0;
};


#endif
