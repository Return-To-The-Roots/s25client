// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "buildings/nobMilitary.h"
#include "gameTypes/BuildingType.h"
#include "helpers/EnumArray.h"

namespace AIJH {

inline unsigned CalculateMilitaryBuildingProtectionValue(
  const nobMilitary& building, const helpers::EnumArray<unsigned, BuildingType>& buildingScores)
{
    unsigned value = 0;
    for(const BuildingType buildingType : building.GetBuildingsLostOnCapture())
        value += buildingScores[buildingType];
    return value;
}

} // namespace AIJH
