// $Id: AIPlayer.cpp 9577 2015-01-23 08:28:23Z marcus $
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


#include "main.h"
#include "AIPlayer.h"
#include "GameClientPlayer.h"
#include "GameWorld.h"
#include "GameCommands.h"

AIPlayer::AIPlayer(const unsigned char playerid, const GameWorldBase* const gwb, const GameClientPlayer* const player,
                   const GameClientPlayerList* const players, const GlobalGameSettings* const ggs,
                   const AI::Level level) : AIBase(playerid, gwb, player, players, ggs, level)
{
}


/// Wird jeden GF aufgerufen und die KI kann hier entsprechende Handlungen vollziehen
void AIPlayer::RunGF(const unsigned gf, bool gfisnwf)
{
}
