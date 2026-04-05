// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CountryEventLogger.h"

#include "EventLogBatchWriter.h"
#include <sstream>

namespace {

EventLogBatchWriter gCountryLog(EventLoggerType::Country, "country_log.csv", "gameframe,playerId,change");

} // namespace

namespace CountryEventLogger {

void LogCountrySizeChange(unsigned gf, unsigned char playerId, int change)
{
    std::ostringstream line;
    line << gf << "," << static_cast<unsigned>(playerId + 1) << "," << change;
    gCountryLog.Append(gf, line.str());
}

} // namespace CountryEventLogger
