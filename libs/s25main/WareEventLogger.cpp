// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "WareEventLogger.h"

#include "ai/aijh/StatsConfig.h"
#include <boost/filesystem/path.hpp>
#include <fstream>

namespace {

std::ofstream OpenWaresLog()
{
    if(STATS_CONFIG.disableEventLogging || STATS_CONFIG.statsPath.empty())
        return {};
    const boost::filesystem::path path = boost::filesystem::path(STATS_CONFIG.statsPath) / "wares_log.csv";
    return std::ofstream(path.string(), std::ios::app);
}

} // namespace

namespace WareEventLogger {

void LogInventoryChange(unsigned gf, unsigned char playerId, GoodType good, int count)
{
    std::ofstream log = OpenWaresLog();
    if(!log)
        return;

    if(log.tellp() == 0)
        log << "gameframe,playerId,goodtype,count" << std::endl;

    log << gf << "," << static_cast<unsigned>(playerId + 1) << "," << GOOD_NAMES_1.at(good) << "," << count
        << std::endl;
}

} // namespace WareEventLogger
