// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TroopsLimitEventLogger.h"

#include "EventLogBatchWriter.h"
#include "gameData/BuildingConsts.h"
#include <sstream>

namespace {

EventLogBatchWriter gTroopsLimitLog(EventLoggerType::TroopsLimit, "troops_limit_log.csv",
                                    "gameframe,playerId,newLimit,buildingType,buildingId");

} // namespace

namespace TroopsLimitEventLogger {

void LogLimitChange(unsigned gf, unsigned char playerId, unsigned newLimit, BuildingType buildingType, unsigned buildingId)
{
    std::ostringstream line;
    line << gf << "," << static_cast<unsigned>(playerId + 1) << "," << newLimit << ","
         << BUILDING_NAMES_1.at(buildingType) << "," << buildingId;
    gTroopsLimitLog.Append(gf, line.str());
}

} // namespace TroopsLimitEventLogger
