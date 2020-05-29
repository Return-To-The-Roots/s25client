// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef TexturesHelper_h__
#define TexturesHelper_h__

#include "gameData/TerrainDesc.h"

namespace rttr { namespace mapGenerator {

    uint8_t ByHumidity(const TerrainDesc& terrain);

    bool IsWater(const TerrainDesc& terrain);

    bool IsLand(const TerrainDesc& terrain);

    bool IsShipableWater(const TerrainDesc& terrain);

    bool IsBuildableLand(const TerrainDesc& terrain);

    bool IsCoastTerrain(const TerrainDesc& terrain);

    bool IsBuildableCoast(const TerrainDesc& terrain);

    bool IsBuildableMountain(const TerrainDesc& terrain);

    bool IsMinableMountain(const TerrainDesc& terrain);

    bool IsSnowOrLava(const TerrainDesc& terrain);

    bool IsMountainOrSnowOrLava(const TerrainDesc& terrain);

}} // namespace rttr::mapGenerator

#endif // TexturesHelper_h__
