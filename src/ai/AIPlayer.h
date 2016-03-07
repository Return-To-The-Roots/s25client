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
#ifndef AIPLAYER_H_INCLUDED
#define AIPLAYER_H_INCLUDED

#pragma once

#include "AIBase.h"
class GameClientPlayerList;
class GameWorldBase;
class GlobalGameSettings;

/// Klasse für die standardmäßige (vorerst) KI
class AIPlayer : public AIBase
{
    public:
        AIPlayer(const unsigned char playerid, const GameWorldBase& gwb, const GameClientPlayer& player,
                 const GameClientPlayerList& players, const GlobalGameSettings& ggs,
                 const AI::Level level);

        /// Wird jeden GF aufgerufen und die KI kann hier entsprechende Handlungen vollziehen
        /// gf ist die GF-Zahl vom Spiel
        void RunGF(const unsigned gf, bool gfisnwf) override;
};

#endif //!AIPLAYER_H_INCLUDED
