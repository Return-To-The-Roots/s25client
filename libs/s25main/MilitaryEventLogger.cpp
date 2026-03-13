// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MilitaryEventLogger.h"

#include "EventLogBatchWriter.h"
#include "gameData/BuildingConsts.h"
#include "gameTypes/JobTypes.h"
#include <array>
#include <sstream>

namespace {

constexpr std::array<char, NUM_SOLDIER_RANKS> kRankLabels = {'P', 'F', 'S', 'O', 'G'};
EventLogBatchWriter gMilitaryLog("military_log.csv", "gameframe,playerId,event,rank,buildingType,buildingId,count");

char RankLabel(unsigned char rank)
{
    if(rank < kRankLabels.size())
        return kRankLabels[rank];
    return '?';
}

void LogEvent(unsigned gf, unsigned char playerId, const char* eventName, unsigned char rank, BuildingType buildingType,
              unsigned buildingId, unsigned count)
{
    std::ostringstream line;
    line << gf << "," << static_cast<unsigned>(playerId + 1) << "," << eventName << "," << RankLabel(rank) << ","
         << BUILDING_NAMES_1.at(buildingType) << "," << buildingId << "," << count;
    gMilitaryLog.Append(gf, line.str());
}

} // namespace

namespace MilitaryEventLogger {

void LogRecruit(unsigned gf, unsigned char playerId, unsigned char rank, BuildingType buildingType, unsigned buildingId,
                unsigned count)
{
    if(count == 0)
        return;
    LogEvent(gf, playerId, "rec", rank, buildingType, buildingId, count);
}

void LogInitialInventoryRecruits(unsigned gf, unsigned char playerId, BuildingType buildingType, unsigned buildingId,
                                 const Inventory& inventory)
{
    for(const Job soldierJob : SOLDIER_JOBS)
    {
        const unsigned count = inventory[soldierJob];
        if(count == 0)
            continue;

        LogRecruit(gf, playerId, static_cast<unsigned char>(getSoldierRank(soldierJob)), buildingType, buildingId,
                   count);
    }
}

void LogUpgrade(unsigned gf, unsigned char playerId, unsigned char newRank, BuildingType buildingType,
                unsigned buildingId, unsigned count)
{
    if(count == 0)
        return;
    LogEvent(gf, playerId, "upg", newRank, buildingType, buildingId, count);
}

void LogLoss(unsigned gf, unsigned char playerId, unsigned char rank, BuildingType targetType, unsigned targetId,
             unsigned count)
{
    if(count == 0)
        return;
    LogEvent(gf, playerId, "los", rank, targetType, targetId, count);
}

void LogDeployment(unsigned gf, unsigned char playerId, unsigned char rank, BuildingType buildingType,
                   unsigned buildingId)
{
    LogEvent(gf, playerId, "dep", rank, buildingType, buildingId, 1);
}

void LogUndeployment(unsigned gf, unsigned char playerId, unsigned char rank, BuildingType buildingType,
                     unsigned buildingId)
{
    LogEvent(gf, playerId, "und", rank, buildingType, buildingId, 1);
}

} // namespace MilitaryEventLogger
