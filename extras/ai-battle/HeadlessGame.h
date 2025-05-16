// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Game.h"
#include "Replay.h"
#include "ai/AIPlayer.h"
#include "gameTypes/AIInfo.h"
#include <boost/filesystem.hpp>
#include <chrono>
#include <limits>
#include <vector>

class GameWorld;
class GlobalGameSettings;
class EventManager;

/// Run an ai-only game without user-interface.
class HeadlessGame
{
public:
    HeadlessGame(const GlobalGameSettings& ggs, const boost::filesystem::path& map, const std::vector<AI::Info>& ais);
    ~HeadlessGame();

    void Run(unsigned maxGF = std::numeric_limits<unsigned>::max());
    void Close();

    void RecordReplay(const boost::filesystem::path& path, unsigned random_init);
    void SaveGame(const boost::filesystem::path& path) const;

private:
    void PrintState();

    boost::filesystem::path map_;
    Game game_;
    GameWorld& world_;
    EventManager& em_;
    std::vector<std::unique_ptr<AIPlayer>> players_;

    Replay replay_;
    boost::filesystem::path replayPath_;

    unsigned lastReportGf_ = 0;
    std::chrono::steady_clock::time_point gameStartTime_;

    std::string toPaddedString(unsigned int value, int width);
};
