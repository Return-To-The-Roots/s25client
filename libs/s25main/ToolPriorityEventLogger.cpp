// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ToolPriorityEventLogger.h"

#include "ai/aijh/StatsConfig.h"
#include "helpers/EnumRange.h"
#include <boost/filesystem/path.hpp>
#include <fstream>

namespace {

std::ofstream OpenToolPriorityLog()
{
    if(STATS_CONFIG.disableEventLogging || STATS_CONFIG.statsPath.empty())
        return {};
    const boost::filesystem::path path = boost::filesystem::path(STATS_CONFIG.statsPath) / "tool_priority.csv";
    return std::ofstream(path.string(), std::ios::app);
}

} // namespace

namespace ToolPriorityEventLogger {

void LogToolPriorityChanges(unsigned gf, unsigned char playerId, const ToolSettings& newSettings)
{
    std::ofstream log = OpenToolPriorityLog();
    if(!log)
        return;

    if(log.tellp() == 0)
    {
        log << "gameframe,playerId";
        for(const auto tool : helpers::enumRange<Tool>())
            log << "," << TOOL_NAMES_1.at(tool);
        log << std::endl;
    }

    log << gf << "," << static_cast<unsigned>(playerId + 1);
    for(const auto tool : helpers::enumRange<Tool>())
    {
        log << "," << static_cast<unsigned>(newSettings[tool]);
    }
    log << std::endl;
}

} // namespace ToolPriorityEventLogger
