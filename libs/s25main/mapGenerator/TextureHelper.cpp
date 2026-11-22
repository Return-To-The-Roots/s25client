// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/TextureHelper.h"

namespace rttr::mapGenerator {

uint8_t byHumidity(const TerrainDesc& terrain)
{
    return terrain.humidity;
}

bool isWater(const TerrainDesc& terrain)
{
    return terrain.kind == TerrainKind::Water;
}

bool isSwamp(const TerrainDesc& terrain)
{
    return !terrain.is(ETerrain::Walkable) && terrain.humidity > 0;
}

bool isImpassable(const TerrainDesc& terrain)
{
    return terrain.flags == ETerrain::Unwalkable;
}

bool isLand(const TerrainDesc& terrain)
{
    return terrain.kind == TerrainKind::Land;
}

bool isShipableWater(const TerrainDesc& terrain)
{
    return terrain.kind == TerrainKind::Water && terrain.is(ETerrain::Shippable);
}

bool isBuildableLand(const TerrainDesc& terrain)
{
    return terrain.kind == TerrainKind::Land && terrain.is(ETerrain::Buildable);
}

bool isCoastTerrain(const TerrainDesc& terrain)
{
    return terrain.kind == TerrainKind::Land && terrain.is(ETerrain::Walkable) && terrain.getBQ() == TerrainBQ::Flag;
}

bool isBuildableCoast(const TerrainDesc& terrain)
{
    return terrain.getBQ() == TerrainBQ::Castle && terrain.isVital() && terrain.humidity < 100;
}

bool isBuildableMountain(const TerrainDesc& terrain)
{
    return terrain.kind == TerrainKind::Mountain && terrain.is(ETerrain::Buildable);
}

bool isMinableMountain(const TerrainDesc& terrain)
{
    return terrain.kind == TerrainKind::Mountain && terrain.is(ETerrain::Mineable);
}

bool isSnowOrLava(const TerrainDesc& terrain)
{
    return terrain.kind == TerrainKind::Snow || terrain.kind == TerrainKind::Lava;
}

bool isMountainOrSnowOrLava(const TerrainDesc& terrain)
{
    return terrain.kind == TerrainKind::Mountain || isSnowOrLava(terrain);
}

} // namespace rttr::mapGenerator
