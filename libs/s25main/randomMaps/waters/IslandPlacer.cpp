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
#include "randomMaps/waters/IslandPlacer.h"
#include "randomMaps/elevation/Level.h"
#include "randomMaps/elevation/Scaler.h"
#include "randomMaps/terrain/TextureTranslator.h"
#include "randomMaps/algorithm/DistanceField.h"

IslandPlacer::IslandPlacer(HeightSettings& height) : height_(height)
{
    
}

bool IslandPlacer::IsCoastLand(const Map* map, int index)
{
    auto size = map->size;
    auto& rsu = map->textureRsu;
    auto& lsd = map->textureLsd;
    
    auto position = GridUtility::GetPosition(index, size);
    auto neighbors = GridUtility::Neighbors(position, size);
    
    for (auto neigbhor: neighbors)
    {
        int idx = neigbhor.x + neigbhor.y * size.x;
        
        if (Texture::IsWater(rsu[idx]) ||
            Texture::IsWater(lsd[idx]))
        {
            return true;
        }
    }

    return false;
}

TextureType IslandPlacer::TextureFor(unsigned char z, unsigned char sea, unsigned char mountain)
{
    if (z == sea + 1)
    {
        return Coast;
    }
    
    if (z == sea + 2)
    {
        return CoastToGreen1;
    }

    if (z == sea + 3)
    {
        return CoastToGreen2;
    }
    
    if (z == mountain)
    {
        return GrassToMountain;
    }
    
    return TextureTranslator(height_).GetTexture(z, sea, mountain);
}

void IslandPlacer::Place(const std::set<Tile, TileCompare>& coverage, Map* map, unsigned char seaLevel)
{
    auto positions = std::vector<Position>();
    auto indices = std::set<int>();
    auto size = map->size;

    // gather all position indices covered by the island
    for (auto tile: coverage)
    {
        indices.insert(tile.IndexRsu(size));
        indices.insert(tile.IndexLsd(size));
    }
    
    // convert indices into positions
    for (auto index: indices)
    {
        positions.push_back(GridUtility::GetPosition(index, size));
    }

    auto& rsu = map->textureRsu;
    auto& lsd = map->textureLsd;
    
    for (auto tile: coverage)
    {
        rsu[tile.IndexRsu(size)] = TextureType::Coast;
        lsd[tile.IndexLsd(size)] = TextureType::Coast;
    }
    
    auto nodes = (int) positions.size();
    auto distance = DistanceField(IsCoastLand).Compute(map, positions);
    
    int maximum = *std::max_element(distance.begin(), distance.end());
    int maxHeight = std::min(maximum + seaLevel + 1, (int)height_.maximum);

    auto heightSettings = HeightSettings(seaLevel + 1, maxHeight);
    auto height = Scaler(heightSettings).Scale(distance);
    
    for (int i = 0; i < nodes; i++)
    {
        map->z[positions[i].x + positions[i].y * size.x] = height[i];
    }

    auto mountainLevel = Level::Mountain(map->z, 0.2, indices);
    
    for (auto tile: coverage)
    {
        auto rsuIndex = tile.IndexRsu(size);
        auto rsuTexture = TextureFor(map->z[rsuIndex], seaLevel, mountainLevel);

        map->textureRsu[rsuIndex] = rsuTexture;
        
        auto lsdIndex = tile.IndexLsd(size);
        auto lsdTexture = TextureFor(map->z[lsdIndex], seaLevel, mountainLevel);
        
        map->textureLsd[lsdIndex] = lsdTexture;
    }
    
    for (int i = 0; i < nodes; i++)
    {
        int index = positions[i].x + positions[i].y * size.x;
        if (map->z[index] >= mountainLevel)
        {
            map->z[index] = min((int)map->z[index] + 1, (int)height_.maximum);
        }
    }
}
