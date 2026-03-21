// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Point.h"
#include "gameTypes/MapCoordinates.h"
#include <cstdint>
#include <vector>

class GameWorldBase;

namespace CountryPlotEventLogger {

struct OwnershipChange
{
    Position localPos;
    uint8_t oldOwner;
    uint8_t newOwner;
};

void LogInitialCountryPlots(unsigned gf, const GameWorldBase& world);
void LogCountryPlotChanges(unsigned gf, const GameWorldBase& world, MapPoint origin, MapExtent size,
                           const std::vector<OwnershipChange>& changes);

} // namespace CountryPlotEventLogger
