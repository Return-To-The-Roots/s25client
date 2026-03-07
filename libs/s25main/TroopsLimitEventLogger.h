// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/BuildingType.h"

namespace TroopsLimitEventLogger {

void LogLimitChange(unsigned gf, unsigned char playerId, unsigned newLimit, BuildingType buildingType, unsigned buildingId);

} // namespace TroopsLimitEventLogger

