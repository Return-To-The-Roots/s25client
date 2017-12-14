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

#pragma once
#ifndef Game_h__
#define Game_h__

#include "EventManager.h"
#include "GlobalGameSettings.h"
#include "world/GameWorld.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

class AIPlayer;

/// Holds all data for a running game
class Game
{
public:
    Game(const GlobalGameSettings& settings, unsigned startGF, const std::vector<PlayerInfo>& players);
    Game(const GlobalGameSettings& settings, EventManager* em, const std::vector<PlayerInfo>& players);
    ~Game();

    const GlobalGameSettings ggs;
    boost::interprocess::unique_ptr<EventManager, Deleter<EventManager> > em;
    GameWorld world;
    boost::ptr_vector<AIPlayer> aiPlayers;

    /// Does the remaining initializations for starting the game
    void Start(bool startFromSave);
    void RunGF();
    bool IsGameFinished() const { return gameFinished; }

private:
    /// Updates the statistics
    void StatisticStep();
    /// Check if the objective was reached (if set)
    void CheckObjective();
    bool gameFinished;
};

#endif // Game_h__
