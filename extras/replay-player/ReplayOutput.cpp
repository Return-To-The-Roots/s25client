// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ReplayOutput.h"
#include "GamePlayer.h"
#include "HeadlessReplay.h"
#include "random/Random.h"
#include "random/randomIO.h"
#include "world/GameWorld.h"
#include "gameTypes/TeamTypes.h"
#include "gameData/NationConsts.h"
#include <boost/nowide/iostream.hpp>
#include <cstdio>

namespace bnw = boost::nowide;

namespace {
std::string teamStr(Team t)
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
} // namespace

void printInitialInfo(const HeadlessReplay& replay)
{
    const Replay& file = replay.getReplay();
    const GameWorld& world = replay.getWorld();
    const MapPoint mapSize = world.GetSize();

    bnw::cout << "\n";
    bnw::cout << "Replay:   " << file.GetMapName() << "\n";
    bnw::cout << "Version:  " << static_cast<unsigned>(file.GetMajorVersion()) << "."
              << static_cast<unsigned>(file.GetMinorVersion()) << "\n";
    bnw::cout << "Type:     " << (replay.isFromSavegame() ? "savegame replay" : "new-game replay") << "\n";
    bnw::cout << "Map size: " << mapSize.x << " x " << mapSize.y << "\n";
    bnw::cout << "Seed:     " << file.getSeed() << "\n";
    bnw::cout << "GF range: " << replay.getStartGF() << " - " << file.GetLastGF() << "  ("
              << (file.GetLastGF() + 1 - replay.getStartGF()) << " GFs)\n";
    if(world.HasLua())
        bnw::cout << "Lua:      script loaded from replay\n";
    if(!replay.isFromSavegame() && !file.usesFishFix())
        bnw::cout << "Note:     fish fix skipped, this replay predates version 8.3\n";
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

void printDesync(const ReplayDesync& desync, bool verbose)
{
    bnw::cerr << "\nDesync at GF " << desync.gf << ":\n"
              << "  actual:  " << desync.actual << "\n"
              << "  stored:  " << desync.expected << "\n";
    if(verbose)
    {
        for(const auto& entry : RANDOM.GetAsyncLog())
            bnw::cerr << "  " << entry << "\n";
    }
    bnw::cerr << "FAIL: Desync detected.\n";
}
