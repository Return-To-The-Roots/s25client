// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/Inventory.h"
#include "gameTypes/BuildingType.h"

namespace MilitaryEventLogger {

void LogRecruit(unsigned gf, unsigned char playerId, unsigned char rank, BuildingType buildingType, unsigned buildingId,
                unsigned count);
void LogInitialInventoryRecruits(unsigned gf, unsigned char playerId, BuildingType buildingType, unsigned buildingId,
                                 const Inventory& inventory);
void LogUpgrade(unsigned gf, unsigned char playerId, unsigned char newRank, BuildingType buildingType,
                unsigned buildingId, unsigned count = 1);
void LogLoss(unsigned gf, unsigned char playerId, unsigned char rank, BuildingType targetType, unsigned targetId,
             unsigned count = 1);
void LogDeployment(unsigned gf, unsigned char playerId, unsigned char rank, BuildingType buildingType,
                   unsigned buildingId);
void LogUndeployment(unsigned gf, unsigned char playerId, unsigned char rank, BuildingType buildingType,
                     unsigned buildingId);

} // namespace MilitaryEventLogger
