// Copyright (C) 2017 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/TextureHelper.h"

namespace rttr { namespace mapGenerator {

    uint8_t ByHumidity(const TerrainDesc& terrain) { return terrain.humidity; }

    bool IsWater(const TerrainDesc& terrain) { return terrain.kind == TerrainKind::Water; }

    bool IsSwamp(const TerrainDesc& terrain) { return !terrain.Is(ETerrain::Walkable) && terrain.humidity > 0; }

    bool IsLand(const TerrainDesc& terrain) { return terrain.kind == TerrainKind::Land; }

    bool IsShipableWater(const TerrainDesc& terrain)
    {
        return terrain.kind == TerrainKind::Water && terrain.Is(ETerrain::Shippable);
    }

    bool IsBuildableLand(const TerrainDesc& terrain)
    {
        return terrain.kind == TerrainKind::Land && terrain.Is(ETerrain::Buildable);
    }

    bool IsCoastTerrain(const TerrainDesc& terrain)
    {
        return terrain.kind == TerrainKind::Land && terrain.Is(ETerrain::Walkable)
               && terrain.GetBQ() == TerrainBQ::Flag;
    }

    bool IsBuildableCoast(const TerrainDesc& terrain)
    {
        return terrain.GetBQ() == TerrainBQ::Castle && terrain.IsVital() && terrain.humidity < 100;
    }

    bool IsBuildableMountain(const TerrainDesc& terrain)
    {
        return terrain.kind == TerrainKind::Mountain && terrain.Is(ETerrain::Buildable);
    }

    bool IsMinableMountain(const TerrainDesc& terrain)
    {
        return terrain.kind == TerrainKind::Mountain && terrain.Is(ETerrain::Mineable);
    }

    bool IsSnowOrLava(const TerrainDesc& terrain)
    {
        return terrain.kind == TerrainKind::Snow || terrain.kind == TerrainKind::Lava;
    }

    bool IsMountainOrSnowOrLava(const TerrainDesc& terrain)
    {
        return terrain.kind == TerrainKind::Mountain || IsSnowOrLava(terrain);
    }

}} // namespace rttr::mapGenerator
