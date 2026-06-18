// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ILocalGameState.h"

#include <chrono>
#include <string>

class Game;
class GameWorld;
class Replay;

#if defined(__MINGW32__) && !defined(__clang__)
void printConsole(const char* fmt, ...) __attribute__((format(gnu_printf, 1, 2)));
#elif defined __GNUC__
void printConsole(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
#else
void printConsole(const char* fmt, ...);
#endif

struct HeadlessGameState : ILocalGameState
{
    unsigned GetPlayerId() const override { return 0; }
    bool IsHost() const override { return true; }
    std::string FormatGFTime(unsigned) const override { return ""; }
    void SystemChat(const std::string& msg) override;
};

struct ReplayStatus
{
    const Game& game;
    const GameWorld& world;
    unsigned totalGFs;
    std::chrono::steady_clock::time_point startTime;
    unsigned lastReportGF = 0;
};

void printTable(const ReplayStatus& s);
void printInitialInfo(const Replay& replay, const GameWorld& world, bool isSavegame, unsigned startGF);
