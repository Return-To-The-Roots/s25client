// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameData/MaxPlayers.h"
#include "gameTypes/JobTypes.h"
#include <array>
#include <cstdint>

class GamePlayer;

struct PlayerMilitaryStats
{
    uint32_t recruitsAcquired = 0;
    uint32_t upgrades = 0;
    std::array<uint32_t, NUM_SOLDIER_RANKS> lossesByRank{};
    double densityFar = 0.0;
    double densityMid = 0.0;
    double densityHarbor = 0.0;
    double densityNear = 0.0;
    double densityTotal = 0.0;
};

namespace MilitaryStatsHolder {

void ReportRecruitAcquire(unsigned char playerId, unsigned count = 1);
void ReportUnitLost(unsigned char playerId, unsigned char rank, unsigned count = 1);
void ReportUnitUpgrade(unsigned char playerId, unsigned count = 1);
void RefreshDensities(const GamePlayer& player);
const PlayerMilitaryStats& GetPlayerStats(unsigned char playerId);
void Reset();

} // namespace MilitaryStatsHolder
