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

#include "mapGenerator/Islands.h"
#include "mapGenerator/HeightMap.h"
#include "mapGenerator/DistanceByProperty.h"
#include "mapGenerator/Textures.h"

#include <stack>

namespace rttr {
namespace mapGenerator {

WaterMap CreateWaterMap(HeightMap& heightMap, Height seaLevel)
{
    const int n = heightMap.size();

    WaterMap waterMap(n);
    
    for (int i = 0; i < n; ++i)
    {
        waterMap[i] = (heightMap[i] <= seaLevel);
        
        if (waterMap[i])
        {
            heightMap[i] = seaLevel;
        }
    }
    
    return waterMap;
}

WaterMap CreateWaterMap(const Map_& map, TextureMapping_& mapping)
{
    const auto water = mapping.water;
    const auto& rsu = map.textureRsu;
    const auto& lsd = map.textureLsd;
    
    int tiles = map.size.x * map.size.y;
    WaterMap waterMap(tiles);
    
    for (int i = 0; i < tiles; i++)
    {
        waterMap[i] = rsu[i] == water && lsd[i] == water;
    }
    
    return waterMap;
}

Position FindNextPosition(const std::vector<bool>& excludedPositions, const MapExtent& size)
{
    for (auto i = 0; i < size.x * size.y; i++)
    {
        if (!excludedPositions[i])
        {
            return GridPosition(i, size);
        }
    }
    return Position::Invalid();
}

Islands FindIslands(const std::vector<bool>& waterMap, const MapExtent& size, unsigned minimumTilesPerIsland)
{
    Islands islands;
    
    auto excludedPositions(waterMap);
    auto currentPosition = FindNextPosition(excludedPositions, size);
    
    while (currentPosition.isValid())
    {
        auto island = GridCollect(currentPosition, size, waterMap);

        for (const auto& tile : island)
        {
            excludedPositions[tile.x + tile.y * size.x] = true;
        }
        
        currentPosition = FindNextPosition(excludedPositions, size);
        
        if (island.size() >= minimumTilesPerIsland)
        {
            islands.push_back(island);
        }
    }
    
    return islands;
}

Position FindSuitableIslandLocation(const Map_& map, const std::vector<int>& landDistance)
{
    auto maximumDistance = std::max_element(landDistance.begin(), landDistance.end());
    auto maximumIndex = std::distance(landDistance.begin(), maximumDistance);

    return GridPosition(maximumIndex, map.size);
}

void PlaceIsland(const IslandCoverage& coverage, Map_& map, TextureMapping_& mapping)
{
    const int sea = map.sea;
    const int peak = map.height.max;
    const Texture coast = mapping.coast;
    
    auto positions = std::vector<Position>();
    auto indices = std::set<int>();
    auto size = map.size;

    // gather all position indices covered by the island
    for (auto tile: coverage)
    {
        indices.insert(tile.IndexRsu(size));
        indices.insert(tile.IndexLsd(size));
    }
    
    // convert indices into positions
    for (auto index: indices)
    {
        positions.push_back(GridPosition(index, size));
    }

    auto& rsu = map.textureRsu;
    auto& lsd = map.textureLsd;
    
    for (auto tile: coverage)
    {
        rsu[tile.IndexRsu(size)] = coast;
        lsd[tile.IndexLsd(size)] = coast;
    }
    
    auto nodes = static_cast<int>(positions.size());
    auto isCoast = [&map, &mapping](int index) { return IsCoastLand(map, mapping, index); };
    auto distance = DistanceByProperty(positions, size, isCoast);
    
    int maxDistance = *std::max_element(distance.begin(), distance.end());
    int maxHeight = std::min(maxDistance + sea + 1, peak);

    auto height = HeightFromDistance(distance, Range(sea + 1, maxHeight));
    
    for (int i = 0; i < nodes; i++)
    {
        map.z[positions[i].x + positions[i].y * size.x] = height[i];
    }
    
    auto mountains = MountainLevel(map.z, 0.2, indices);
    auto textures = mapping.MapHeightsToTerrains(peak, sea, mountains);
    
    for (auto tile: coverage)
    {
        auto rsuIndex = tile.IndexRsu(size);
        auto rsuTexture = textures[map.z[rsuIndex]];

        map.textureRsu[rsuIndex] = rsuTexture;
        
        auto lsdIndex = tile.IndexLsd(size);
        auto lsdTexture = textures[map.z[lsdIndex]];
        
        map.textureLsd[lsdIndex] = lsdTexture;
    }
}

Island CreateIsland(RandomUtility& rnd,
                    int islandSize,
                    int islandLength,
                    int distanceToLand,
                    Map_& map,
                    TextureMapping_& mapping)
{
    auto islandTriangles = islandSize * 2;
    auto size = map.size;
    
    auto isLand = [&map, &mapping] (int index) {
        return
            !mapping.IsWater(map.textureRsu[index]) ||
            !mapping.IsWater(map.textureLsd[index]);
    };
    
    const auto distance = DistanceByProperty(size, isLand);
    const auto location = FindSuitableIslandLocation(map, distance);
    
    IslandCoverage visited;
    IslandCoverage coverage;
    
    std::stack<Tile> skeleton;
    
    std::set<int> rsuIndices;
    std::set<int> lsdIndices;

    // Generate a skeleton shape for the island by adding
    // tiles in randomly shaped line.

    skeleton.push(location);

    int currentSize = 0;

    while (coverage.size() < static_cast<unsigned>(islandLength) &&
           currentSize < islandTriangles &&
           !skeleton.empty())
    {
        auto current = skeleton.top();

        coverage.insert(current);
        rsuIndices.insert(current.IndexRsu(size));
        lsdIndices.insert(current.IndexLsd(size));
        
        currentSize = rsuIndices.size() + lsdIndices.size();

        skeleton.pop();

        auto neighbors = current.Neighbors(size);
        auto indices = rnd.ShuffledRange(8);
        
        for (int i = 0; i < 8; ++i)
        {
            auto neighbor = neighbors[indices[i]];
            if (distance[neighbor.IndexRsu(size)] > distanceToLand &&
                distance[neighbor.IndexLsd(size)] > distanceToLand)
            {
                if (visited.insert(neighbor).second)
                {
                    skeleton.push(neighbor);
                }
            }
        }
    }
    
    // Just add tiles around the skeleton shape until the island reached
    // the desired number of tiles.
    
    currentSize = rsuIndices.size() + lsdIndices.size();

    while (currentSize < islandTriangles)
    {
        int index = 0;
        std::vector<Tile> coast(coverage.size());
        
        for (auto tile : coverage)
        {
            coast[index] = tile;
            index++;
        }
        
        int updatedSize = currentSize;

        for (auto t : coast)
        {
            if (updatedSize >= islandTriangles)
            {
                break;
            }
            
            auto neighbors = t.Neighbors(size);

            for (auto neighbor : neighbors)
            {
                if (updatedSize >= islandTriangles)
                {
                    break;
                }

                if (distance[neighbor.IndexRsu(size)] > distanceToLand &&
                    distance[neighbor.IndexLsd(size)] > distanceToLand)
                {
                    if (coverage.insert(neighbor).second)
                    {
                        rsuIndices.insert(neighbor.IndexRsu(size));
                        lsdIndices.insert(neighbor.IndexLsd(size));
                        
                        updatedSize = rsuIndices.size() + lsdIndices.size();
                    }
                }
            }
        }
        
        if (currentSize != updatedSize)
        {
            currentSize = updatedSize;
        }
        else
        {
            break;
        }
    }

    PlaceIsland(coverage, map, mapping);

    return TileSetToPositions(coverage);
}

}}
