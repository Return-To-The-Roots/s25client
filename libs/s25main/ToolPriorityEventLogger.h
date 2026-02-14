// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/SettingsTypes.h"

namespace ToolPriorityEventLogger {

void LogToolPriorityChanges(unsigned gf, unsigned char playerId, const ToolSettings& newSettings);

} // namespace ToolPriorityEventLogger
