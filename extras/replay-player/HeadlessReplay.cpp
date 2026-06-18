// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "HeadlessReplay.h"
#include "EventManager.h"
#include "Game.h"
#include "GamePlayer.h"
#include "Replay.h"
#include "world/GameWorld.h"
#include "gameTypes/TeamTypes.h"
#include "gameData/GameConsts.h"
#include "gameData/NationConsts.h"

#include <boost/nowide/iostream.hpp>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <sstream>

#ifdef _WIN32
#    include <windows.h>
#endif

namespace bnw = boost::nowide;

#ifdef _WIN32
static HANDLE setupStdOut()
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleMode(h, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleOutputCP(65001);
    return h;
}
#endif

void printConsole(const char* fmt, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    const int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if(len > 0 && static_cast<size_t>(len) < sizeof(buffer))
    {
#ifdef _WIN32
        static HANDLE h = setupStdOut();
        WriteConsoleA(h, buffer, len, nullptr, nullptr);
#else
        bnw::cout << buffer;
#endif
    }
}

static std::string fmtClock(const std::chrono::milliseconds& time)
{
    char buf[32];
    const auto h = std::chrono::duration_cast<std::chrono::hours>(time);
    const auto m = std::chrono::duration_cast<std::chrono::minutes>(time % std::chrono::hours(1));
    const auto s = std::chrono::duration_cast<std::chrono::seconds>(time % std::chrono::minutes(1));
    snprintf(buf, sizeof(buf), "%03ld:%02ld:%02ld", static_cast<long>(h.count()), static_cast<long>(m.count()),
             static_cast<long>(s.count()));
    return buf;
}

static std::string fmtNum(unsigned n)
{
    std::ostringstream ss;
    ss.imbue(std::locale(""));
    ss << std::fixed << n;
    return ss.str();
}

void HeadlessGameState::SystemChat(const std::string& msg)
{
    bnw::cout << "[sys] " << msg << "\n";
}

void printTable(const ReplayStatus& s)
{
    const unsigned curGF = s.game.em_->GetCurrentGF();
    const auto wallElapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - s.startTime);
    const auto gameElapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(SPEED_GF_LENGTHS[GameSpeed::Normal] * curGF);
    const unsigned gfPerSec = curGF - s.lastReportGF;
    const unsigned numPlayers = s.world.GetNumPlayers();

    static bool firstCall = true;
    if(!firstCall)
        printConsole("\x1b[%uA", 8 + numPlayers);
    firstCall = false;

    printConsole("┌──────────────────────────┬───────────────────────┬───────────────────────┬────────────────┐\n");
    printConsole("│ GF %10s / %-8s │ Game Clock  %s │ Wall Clock  %s │ %7s GF/sec │\n", fmtNum(curGF).c_str(),
                 fmtNum(s.totalGFs).c_str(), fmtClock(gameElapsed).c_str(), fmtClock(wallElapsed).c_str(),
                 fmtNum(gfPerSec).c_str());
    printConsole("└──────────────────────────┴───────────────────────┴───────────────────────┴────────────────┘\n");
    printConsole("\n");

    printConsole("┌────────────────────────────┬────────────────────┬───────────────┬────────────┬────────────┐\n");
    printConsole("│ %-26s │ %-18s │ %-13s │ %-10s │ %-10s │\n", "Player", "Country", "Buildings", "Military", "Gold");
    printConsole("├────────────────────────────┼────────────────────┼───────────────┼────────────┼────────────┤\n");
    for(unsigned i = 0; i < numPlayers; ++i)
    {
        const GamePlayer& p = s.world.GetPlayer(i);
        printConsole("│ %s%-26s%s │ %18s │ %13s │ %10s │ %10s │\n", p.IsDefeated() ? "\x1b[9m" : "", p.name.c_str(),
                     p.IsDefeated() ? "\x1b[29m" : "",
                     fmtNum(p.GetStatisticCurrentValue(StatisticType::Country)).c_str(),
                     fmtNum(p.GetStatisticCurrentValue(StatisticType::Buildings)).c_str(),
                     fmtNum(p.GetStatisticCurrentValue(StatisticType::Military)).c_str(),
                     fmtNum(p.GetStatisticCurrentValue(StatisticType::Gold)).c_str());
    }
    printConsole("└────────────────────────────┴────────────────────┴───────────────┴────────────┴────────────┘\n");
}

static std::string teamStr(Team t)
{
    switch(t)
    {
        case Team::Team1: return "1";
        case Team::Team2: return "2";
        case Team::Team3: return "3";
        case Team::Team4: return "4";
        case Team::None: return "-";
        default: return "R";
    }
}

void printInitialInfo(const Replay& replay, const GameWorld& world, bool isSavegame, unsigned startGF)
{
    const MapPoint mapSize = world.GetSize();
    bnw::cout << "\n";
    bnw::cout << "Replay:   " << replay.GetMapName() << "\n";
    bnw::cout << "Version:  " << static_cast<unsigned>(replay.GetMajorVersion()) << "."
              << static_cast<unsigned>(replay.GetMinorVersion()) << "\n";
    bnw::cout << "Type:     " << (isSavegame ? "savegame replay" : "new-game replay") << "\n";
    bnw::cout << "Map size: " << mapSize.x << " x " << mapSize.y << "\n";
    bnw::cout << "Seed:     " << replay.getSeed() << "\n";
    bnw::cout << "GF range: " << startGF << " - " << replay.GetLastGF() << "  (" << (replay.GetLastGF() - startGF)
              << " GFs)\n";
    bnw::cout << "\n";

    bnw::cout << "Players:\n";
    bnw::cout << "  #  Name                     Nation       Team\n";
    bnw::cout << "  ─────────────────────────────────────────────\n";
    for(unsigned i = 0; i < world.GetNumPlayers(); ++i)
    {
        const GamePlayer& p = world.GetPlayer(i);
        char buf[80];
        snprintf(buf, sizeof(buf), "  %-2u %-24s %-12s %s\n", i, p.name.c_str(), NationNames[p.nation],
                 teamStr(p.team).c_str());
        bnw::cout << buf;
    }
    bnw::cout << "\n";
    bnw::cout.flush();
}
