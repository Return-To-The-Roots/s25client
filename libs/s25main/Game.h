// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GlobalGameSettings.h"
#include "world/GameWorld.h"
#include <boost/ptr_container/ptr_vector.hpp>
#include <memory>

class AIPlayer;

/// Holds all data for a running game
class Game
{
public:
    Game(GlobalGameSettings settings, unsigned startGF, const std::vector<PlayerInfo>& players);
    Game(GlobalGameSettings settings, std::unique_ptr<EventManager> em, const std::vector<PlayerInfo>& players);
    ~Game();

    const GlobalGameSettings ggs_;
    std::unique_ptr<EventManager> em_;
    GameWorld world_;
    boost::ptr_vector<AIPlayer> aiPlayers_;

    /// Does the remaining initializations for starting the game
    void Start(bool startFromSave);
    void RunGF();
    bool IsStarted() const { return started_; }
    bool IsGameFinished() const { return finished_; }
    AIPlayer* GetAIPlayer(unsigned id);
    void AddAIPlayer(std::unique_ptr<AIPlayer> newAI);
    void SetLua(std::unique_ptr<LuaInterfaceGame> newLua);

private:
    /// Updates the statistics
    void StatisticStep();
    /// Check if the objective was reached (if set)
    void CheckObjective();

    bool started_, finished_;
    std::unique_ptr<LuaInterfaceGame> lua;
};
