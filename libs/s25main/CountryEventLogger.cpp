// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CountryEventLogger.h"

#include "ai/aijh/StatsConfig.h"
#include <boost/filesystem/path.hpp>
#include <fstream>

namespace {

std::ofstream OpenCountryLog()
{
    if(STATS_CONFIG.disableEventLogging || STATS_CONFIG.statsPath.empty())
        return {};
    const boost::filesystem::path path = boost::filesystem::path(STATS_CONFIG.statsPath) / "country_log.csv";
    return std::ofstream(path.string(), std::ios::app);
}

} // namespace

namespace CountryEventLogger {

void LogCountrySizeChange(unsigned gf, unsigned char playerId, int change)
{
    std::ofstream log = OpenCountryLog();
    if(!log)
        return;

    if(log.tellp() == 0)
        log << "gameframe,playerId,change" << std::endl;

    log << gf << "," << static_cast<unsigned>(playerId + 1) << "," << change << std::endl;
}

} // namespace CountryEventLogger

