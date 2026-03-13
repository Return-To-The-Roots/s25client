// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ToolPriorityEventLogger.h"

#include "EventLogBatchWriter.h"
#include "helpers/EnumRange.h"
#include <sstream>

namespace {

const std::string kToolPriorityHeader = [] {
    std::ostringstream header;
    header << "gameframe,playerId";
    for(const auto tool : helpers::enumRange<Tool>())
        header << "," << TOOL_NAMES_1.at(tool);
    return header.str();
}();

EventLogBatchWriter gToolPriorityLog("tool_priority.csv", kToolPriorityHeader);

} // namespace

namespace ToolPriorityEventLogger {

void LogToolPriorityChanges(unsigned gf, unsigned char playerId, const ToolSettings& newSettings)
{
    std::ostringstream line;
    line << gf << "," << static_cast<unsigned>(playerId + 1);
    for(const auto tool : helpers::enumRange<Tool>())
    {
        line << "," << static_cast<unsigned>(newSettings[tool]);
    }

    gToolPriorityLog.Append(gf, line.str());
}

} // namespace ToolPriorityEventLogger
