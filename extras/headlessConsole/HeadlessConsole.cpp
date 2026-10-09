// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "HeadlessConsole.h"
#include "GamePlayer.h"
#include "world/GameWorldBase.h"
#include <boost/nowide/iostream.hpp>
#include <cstdarg>
#include <cstdio>
#include <locale>
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

std::string formatClock(std::chrono::milliseconds time)
{
    char buf[32];
    const auto h = std::chrono::duration_cast<std::chrono::hours>(time);
    const auto m = std::chrono::duration_cast<std::chrono::minutes>(time % std::chrono::hours(1));
    const auto s = std::chrono::duration_cast<std::chrono::seconds>(time % std::chrono::minutes(1));
    snprintf(buf, sizeof(buf), "%03ld:%02ld:%02ld", static_cast<long>(h.count()), static_cast<long>(m.count()),
             static_cast<long>(s.count()));
    return buf;
}

std::string formatNumber(unsigned num)
{
    std::ostringstream ss;
    ss.imbue(std::locale(""));
    ss << std::fixed << num;
    return ss.str();
}

void StatsTablePrinter::print(const HeadlessStats& stats, const GameWorldBase& world)
{
    const unsigned numPlayers = world.GetNumPlayers();
    if(!firstPrint_)
        printConsole("\x1b[%uA", 8 + numPlayers); // Move back up over the previously printed table
    firstPrint_ = false;

    const std::string gf = stats.totalGFs ? formatNumber(stats.currentGF) + " / " + formatNumber(*stats.totalGFs) :
                                            formatNumber(stats.currentGF);

    printConsole("┌──────────────────────────┬───────────────────────┬───────────────────────┬────────────────┐\n");
    printConsole("│ GF %21s │ Game Clock  %s │ Wall Clock  %s │ %7s GF/sec │\n", gf.c_str(),
                 formatClock(stats.gameTime).c_str(), formatClock(stats.wallTime).c_str(),
                 formatNumber(stats.gfPerSecond).c_str());
    printConsole("└──────────────────────────┴───────────────────────┴───────────────────────┴────────────────┘\n");
    printConsole("\n");

    printConsole("┌────────────────────────────┬────────────────────┬───────────────┬────────────┬────────────┐\n");
    printConsole("│ %-26s │ %-18s │ %-13s │ %-10s │ %-10s │\n", "Player", "Country", "Buildings", "Military", "Gold");
    printConsole("├────────────────────────────┼────────────────────┼───────────────┼────────────┼────────────┤\n");
    for(unsigned i = 0; i < numPlayers; ++i)
    {
        const GamePlayer& p = world.GetPlayer(i);
        printConsole("│ %s%-26s%s │ %18s │ %13s │ %10s │ %10s │\n", p.IsDefeated() ? "\x1b[9m" : "", p.name.c_str(),
                     p.IsDefeated() ? "\x1b[29m" : "",
                     formatNumber(p.GetStatisticCurrentValue(StatisticType::Country)).c_str(),
                     formatNumber(p.GetStatisticCurrentValue(StatisticType::Buildings)).c_str(),
                     formatNumber(p.GetStatisticCurrentValue(StatisticType::Military)).c_str(),
                     formatNumber(p.GetStatisticCurrentValue(StatisticType::Gold)).c_str());
    }
    printConsole("└────────────────────────────┴────────────────────┴───────────────┴────────────┴────────────┘\n");
}
