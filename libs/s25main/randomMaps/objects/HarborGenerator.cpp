// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "randomMaps/objects/HarborGenerator.h"
#include "randomMaps/algorithm/BrushSize.h"
#include "randomMaps/algorithm/BrushDirection.h"

#include <cmath>

void HarborGenerator::Build(Map& map, const CoastTile& pos)
{
    HarborParams params(map);
    unsigned char waterLevel = map.z[pos.water.x + pos.water.y * map.size.x];
    
    Brush<HarborParams> flattenBrush(EnsureTerrainIsFlat);
    params.waterLevel = waterLevel + 2;
    flattenBrush.Paint(params, BrushSize::Large(), pos.coast, map.size);
    params.waterLevel = waterLevel + 1;
    flattenBrush.Paint(params, BrushSize::Medium(), pos.coast, map.size);
    params.waterLevel = waterLevel;
    flattenBrush.Paint(params, BrushSize::Small(), pos.coast, map.size);

    Brush<HarborParams> terrainBrush(EnsureTerrainIsBuildable);
    params.texture = CoastToGreen1;
    terrainBrush.Paint(params, BrushSize::Medium(), pos.coast, map.size);

    Brush<HarborParams> harborBrush(EnsureHarborIsBuildable);
    params.texture = CoastToGreen2;
    harborBrush.Paint(params, BrushSize::Small(), pos.coast, map.size);

    Brush<HarborParams> waterBrush(EnsureWaterAccess);
    params.texture = Water;
    waterBrush.Paint(params, GetBrushShape(map, pos.coast), pos.coast, map.size);
}

BrushSettings HarborGenerator::GetBrushShape(Map& map, const Position& vector)
{
    std::vector<BrushSettings> directions = {
        BrushDirection::East(),
        BrushDirection::West(),
        BrushDirection::North(),
        BrushDirection::NorthEast(),
        BrushDirection::NorthWest(),
        BrushDirection::South(),
        BrushDirection::SouthEast(),
        BrushDirection::SouthWest()
    };

    Brush<TextureParams> counter(TextureTileCounter);
    BrushSettings maxShape = BrushDirection::East();

    int maxWaterTiles = 0;

    for (auto dir = directions.begin(); dir != directions.end(); dir++)
    {
        TextureParams params(map, Water);
        counter.Paint(params, *dir, vector, map.size);
        
        if (maxWaterTiles <= params.number)
        {
            maxWaterTiles = params.number;
            maxShape = *dir;
        }
    }
    
    return maxShape;
}

void HarborGenerator::EnsureTerrainIsFlat(HarborParams& params, int index, bool rsu)
{
    if ((params.map.textureRsu[index] != Water && rsu) ||
        (params.map.textureLsd[index] != Water && !rsu))
    {
        params.map.z[index] =
            std::min(params.waterLevel,
                     params.map.z[index]);
    }
    
    params.map.objectInfo[index] = libsiedler2::OI_Empty;
    params.map.objectType[index] = libsiedler2::OT_Empty;
}

void HarborGenerator::EnsureTerrainIsBuildable(HarborParams& params, int index, bool rsu)
{
    if (rsu)
    {
        if (params.map.textureRsu[index] == Coast)
        {
            params.map.textureRsu[index] = params.texture;
        }
    }
    else
    {
        if (params.map.textureLsd[index] == Coast)
        {
            params.map.textureLsd[index] = params.texture;
        }
    }
}

void HarborGenerator::EnsureHarborIsBuildable(HarborParams& params, int index, bool rsu)
{
    if (rsu)
    {
        params.map.textureRsu[index] = params.texture;
        params.map.harborsRsu.insert(index);
    }
    else
    {
        params.map.textureLsd[index] = params.texture;
        params.map.harborsLsd.insert(index);
    }
}

void HarborGenerator::EnsureWaterAccess(HarborParams& params, int index, bool rsu)
{
    if (rsu)
    {
        params.map.textureRsu[index] = params.texture;
    }
    else
    {
        params.map.textureLsd[index] = params.texture;
    }
}

void HarborGenerator::TextureTileCounter(TextureParams& params, int index, bool rsu)
{
    if (rsu)
    {
        if (params.map.textureRsu[index] == params.texture)
        {
            params.number++;
        }
    }
    else
    {
        if (params.map.textureLsd[index] == params.texture)
        {
            params.number++;
        }
    }
}
