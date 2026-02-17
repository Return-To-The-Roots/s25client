// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MilitaryStatsHolder.h"

#include <algorithm>

namespace {

std::array<PlayerMilitaryStats, MAX_PLAYERS> gPlayerMilitaryStats;
const PlayerMilitaryStats kEmptyStats{};

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
