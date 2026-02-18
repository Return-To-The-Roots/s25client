// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MilitaryStatsHolder.h"

#include "GamePlayer.h"
#include "buildings/nobMilitary.h"

#include <algorithm>

namespace {

std::array<PlayerMilitaryStats, MAX_PLAYERS> gPlayerMilitaryStats;
const PlayerMilitaryStats kEmptyStats{};

double CalcDensity(const unsigned soldiers, const unsigned buildings)
{
    return (buildings == 0) ? 0.0 : static_cast<double>(soldiers) / static_cast<double>(buildings);
}

} // namespace

namespace MilitaryStatsHolder {

void ReportRecruitAcquire(const unsigned char playerId, const unsigned count)
{
    if(playerId >= MAX_PLAYERS || count == 0)
        return;
    gPlayerMilitaryStats[playerId].recruitsAcquired += count;
}

void ReportUnitLost(const unsigned char playerId, const unsigned char rank, const unsigned count)
{
    if(playerId >= MAX_PLAYERS || count == 0)
        return;
    const std::size_t rankIdx =
      std::min<std::size_t>(rank, gPlayerMilitaryStats[playerId].lossesByRank.size() - 1u);
    gPlayerMilitaryStats[playerId].lossesByRank[rankIdx] += count;
}

void ReportUnitUpgrade(const unsigned char playerId, const unsigned count)
{
    if(playerId >= MAX_PLAYERS || count == 0)
        return;
    gPlayerMilitaryStats[playerId].upgrades += count;
}

void RefreshDensities(const GamePlayer& player)
{
    const unsigned char playerId = static_cast<unsigned char>(player.GetPlayerId());
    if(playerId >= MAX_PLAYERS)
        return;

    unsigned farBuildings = 0;
    unsigned midBuildings = 0;
    unsigned harborBuildings = 0;
    unsigned nearBuildings = 0;
    unsigned totalBuildings = 0;

    unsigned farSoldiers = 0;
    unsigned midSoldiers = 0;
    unsigned harborSoldiers = 0;
    unsigned nearSoldiers = 0;
    unsigned totalSoldiers = 0;

    for(const nobMilitary* milBld : player.GetBuildingRegister().GetMilitaryBuildings())
    {
        if(!milBld)
            continue;

        const unsigned numTroops = milBld->GetNumTroops();
        ++totalBuildings;
        totalSoldiers += numTroops;

        switch(milBld->GetFrontierDistance())
        {
            case FrontierDistance::Far:
                ++farBuildings;
                farSoldiers += numTroops;
                break;
            case FrontierDistance::Mid:
                ++midBuildings;
                midSoldiers += numTroops;
                break;
            case FrontierDistance::Harbor:
                ++harborBuildings;
                harborSoldiers += numTroops;
                break;
            case FrontierDistance::Near:
                ++nearBuildings;
                nearSoldiers += numTroops;
                break;
        }
    }

    PlayerMilitaryStats& stats = gPlayerMilitaryStats[playerId];
    stats.densityFar = CalcDensity(farSoldiers, farBuildings);
    stats.densityMid = CalcDensity(midSoldiers, midBuildings);
    stats.densityHarbor = CalcDensity(harborSoldiers, harborBuildings);
    stats.densityNear = CalcDensity(nearSoldiers, nearBuildings);
    stats.densityTotal = CalcDensity(totalSoldiers, totalBuildings);
}

const PlayerMilitaryStats& GetPlayerStats(const unsigned char playerId)
{
    if(playerId >= MAX_PLAYERS)
        return kEmptyStats;
    return gPlayerMilitaryStats[playerId];
}

void Reset()
{
    gPlayerMilitaryStats.fill(PlayerMilitaryStats{});
}

} // namespace MilitaryStatsHolder
