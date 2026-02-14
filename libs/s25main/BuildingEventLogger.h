// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/BuildingType.h"

namespace BuildingEventLogger {

void LogConstructionSiteCreated(unsigned gf, unsigned char playerId, BuildingType buildingType, unsigned x, unsigned y);
void MarkConstructionSiteConstructed(const void* sitePtr);
void LogConstructionSiteCancelled(unsigned gf, unsigned char playerId, BuildingType buildingType, unsigned x, unsigned y,
                                  const void* sitePtr);
void LogBuildingConstructed(unsigned gf, unsigned char playerId, BuildingType buildingType, unsigned buildingId,
                           unsigned x, unsigned y);
void LogBuildingDestroyed(unsigned gf, unsigned char playerId, BuildingType buildingType, unsigned buildingId,
                         unsigned x, unsigned y);
void LogBuildingCaptured(unsigned gf, unsigned char playerId, BuildingType buildingType, unsigned buildingId, unsigned x,
                        unsigned y);

} // namespace BuildingEventLogger
