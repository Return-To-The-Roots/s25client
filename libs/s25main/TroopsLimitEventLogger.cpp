// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TroopsLimitEventLogger.h"

#include "ai/aijh/StatsConfig.h"
#include "gameData/BuildingConsts.h"
#include <boost/filesystem/path.hpp>
#include <fstream>

namespace {

std::ofstream OpenTroopsLimitLog()
{
    if(STATS_CONFIG.disableEventLogging || STATS_CONFIG.statsPath.empty())
        return {};
    const boost::filesystem::path path = boost::filesystem::path(STATS_CONFIG.statsPath) / "troops_limit_log.csv";
    return std::ofstream(path.string(), std::ios::app);
}

} // namespace

namespace TroopsLimitEventLogger {

void LogLimitChange(unsigned gf, unsigned char playerId, unsigned newLimit, BuildingType buildingType, unsigned buildingId)
{
    std::ofstream log = OpenTroopsLimitLog();
    if(!log)
        return;

    if(log.tellp() == 0)
        log << "gameframe,playerId,newLimit,buildingType,buildingId" << std::endl;

    log << gf << "," << static_cast<unsigned>(playerId + 1) << "," << newLimit << "," << BUILDING_NAMES_1.at(buildingType)
        << "," << buildingId << std::endl;
}

} // namespace TroopsLimitEventLogger

