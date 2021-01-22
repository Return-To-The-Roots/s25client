// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
