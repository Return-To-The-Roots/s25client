// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "terrainHelpers.h"
#include "gameData/TerrainDesc.h"
#include "gameData/WorldDescription.h"

DescIdx<TerrainDesc> GetWaterTerrain(const WorldDescription& desc)
{
    const auto tWater = desc.terrain.find([](const TerrainDesc& t) {
        return t.kind == TerrainKind::Water && !t.Is(ETerrain::Walkable) && t.Is(ETerrain::Shippable);
    });
    if(!tWater)
        throw std::logic_error("No water"); // LCOV_EXCL_LINE
    return tWater;
}

DescIdx<TerrainDesc> GetLandTerrain(const WorldDescription& desc, const ETerrain property)
{
    const auto tLand =
      desc.terrain.find([property](const TerrainDesc& t) { return t.kind == TerrainKind::Land && t.Is(property); });
    if(!tLand)
        throw std::logic_error("No land"); // LCOV_EXCL_LINE
    return tLand;
}