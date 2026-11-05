// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <optional>
#include <string>

class GameWorldBase;

/// Write to the console, enabling VT100 escapes and UTF-8 output on Windows.
/// On Windows nothing reaches a redirected stdout, e.g. a pipe or log file.
#if defined(__MINGW32__) && !defined(__clang__)
void printConsole(const char* fmt, ...) __attribute__((format(gnu_printf, 1, 2)));
#elif defined __GNUC__
void printConsole(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
#else
void printConsole(const char* fmt, ...);
#endif

/// hhh:mm:ss
std::string formatClock(std::chrono::milliseconds time);
/// Number with the digit grouping of the current locale
std::string formatNumber(unsigned num);

struct HeadlessStats
{
    unsigned currentGF = 0;
    /// Shown next to the current GF when the end is known in advance, as it is for a replay
    std::optional<unsigned> totalGFs;
    std::chrono::milliseconds gameTime{};
    std::chrono::milliseconds wallTime{};
    unsigned gfPerSecond = 0;
};

/// Prints a status table over the one it printed before, so it updates in place.
class StatsTablePrinter
{
public:
    void print(const HeadlessStats& stats, const GameWorldBase& world);

private:
    bool firstPrint_ = true;
};
