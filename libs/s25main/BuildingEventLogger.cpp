// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BuildingEventLogger.h"

#include "ai/aijh/StatsConfig.h"
#include "gameData/BuildingConsts.h"
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <unordered_set>

namespace {

std::unordered_set<const void*> gConstructedSites;

std::ofstream OpenBuildingLog()
{
    if(STATS_CONFIG.statsPath.empty())
        return {};
    const boost::filesystem::path path = boost::filesystem::path(STATS_CONFIG.statsPath) / "building_log.csv";
    return std::ofstream(path.string(), std::ios::app);
}

void LogEvent(unsigned gf, unsigned char playerId, const char* eventName, BuildingType buildingType, unsigned buildingId,
              unsigned x, unsigned y)
{
    std::ofstream log = OpenBuildingLog();
    if(!log)
        return;

    if(log.tellp() == 0)
        log << "gameframe,playerId,event,buildingType,buildingId,x,y" << std::endl;

    log << gf << "," << static_cast<unsigned>(playerId + 1) << "," << eventName << ","
        << BUILDING_NAMES_1.at(buildingType) << "," << buildingId << "," << x << "," << y << std::endl;
}

} // namespace

namespace BuildingEventLogger {

void LogConstructionSiteCreated(unsigned gf, unsigned char playerId, BuildingType buildingType, unsigned x, unsigned y)
{
    LogEvent(gf, playerId, "construction_site_created", buildingType, 0, x, y);
}

void MarkConstructionSiteConstructed(const void* sitePtr)
{
    if(sitePtr)
        gConstructedSites.insert(sitePtr);
}

void LogConstructionSiteCancelled(unsigned gf, unsigned char playerId, BuildingType buildingType, unsigned x, unsigned y,
                                  const void* sitePtr)
{
    if(sitePtr)
    {
        const auto it = gConstructedSites.find(sitePtr);
        if(it != gConstructedSites.end())
        {
            gConstructedSites.erase(it);
            return;
        }
    }
    LogEvent(gf, playerId, "construction_site_cancelled", buildingType, 0, x, y);
}

void LogBuildingConstructed(unsigned gf, unsigned char playerId, BuildingType buildingType, unsigned buildingId,
                            unsigned x, unsigned y)
{
    LogEvent(gf, playerId, "constructed", buildingType, buildingId, x, y);
}

void LogBuildingDestroyed(unsigned gf, unsigned char playerId, BuildingType buildingType, unsigned buildingId,
                          unsigned x, unsigned y)
{
    LogEvent(gf, playerId, "destroyed", buildingType, buildingId, x, y);
}

void LogBuildingCaptured(unsigned gf, unsigned char playerId, BuildingType buildingType, unsigned buildingId, unsigned x,
                         unsigned y)
{
    LogEvent(gf, playerId, "captured", buildingType, buildingId, x, y);
}

} // namespace BuildingEventLogger
