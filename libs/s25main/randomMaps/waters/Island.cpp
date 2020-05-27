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
#include "randomMaps/waters/Island.h"
#include "randomMaps/waters/IslandPositioner.h"
#include "randomMaps/waters/WaterMap.h"
#include "randomMaps/algorithm/DistanceField.h"
#include "randomMaps/algorithm/TileConvert.h"

#include <cmath>
#include <set>
#include <stack>

Island::Island(RandomUtility& rnd, IslandPlacer& placer) : rnd_(rnd), placer_(placer)
{
    size_ = 50;
    length_ = 5;
    dist_ = 4;
}

Island& Island::OfSize(int size)
{
    size_ = size * 2;
    return *this;
}

Island& Island::OfLength(int length)
{
    length_ = length;
    return *this;
}

Island& Island::OfDistance(int dist)
{
    dist_ = dist;
    return *this;
}

bool Island::HasLandNeibghor(Map* map, const Tile& tile)
{
    auto rsu = map->textureRsu;
    auto lsd = map->textureLsd;
    auto size = map->size;

    auto neighbors = tile.Neighbors(size);

    for (auto neighbor : neighbors)
    {
        if (rsu[neighbor.IndexRsu(size)] != TextureType::Water)
        {
            return true;
        }
        
        if (lsd[neighbor.IndexLsd(size)] != TextureType::Water)
        {
            return true;
        }
    }
    
    return false;
}

bool Island::IsLand(const Map* map, int index)
{
    return
        map->textureRsu[index] != Water ||
        map->textureLsd[index] != Water;
}

std::vector<Position> Island::Place(Map* map, unsigned char seaLevel)
{
    auto size = map->size;
    auto waterMap = WaterMap::For(*map);
    auto distance = DistanceField(IsLand).Compute(map);
    auto location = IslandPositioner().FindLocation(map, distance);
    
    std::set<Tile, TileCompare> visited;
    std::set<Tile, TileCompare> coverage;
    std::stack<Tile> skeleton;
    
    std::set<int> rsuIndices;
    std::set<int> lsdIndices;

    // Generate a skeleton shape for the island by adding
    // tiles in randomly shaped line.

    skeleton.push(location);

    int currentSize = 0;

    while (coverage.size() < (unsigned)length_ &&
           currentSize < size_ &&
           !skeleton.empty())
    {
        auto current = skeleton.top();

        coverage.insert(current);
        rsuIndices.insert(current.IndexRsu(size));
        lsdIndices.insert(current.IndexLsd(size));
        
        currentSize = rsuIndices.size() + lsdIndices.size();

        skeleton.pop();

        auto neighbors = current.Neighbors(size);
        auto indices = rnd_.IRand(8);
        
        for (int i = 0; i < 8; ++i)
        {
            auto neighbor = neighbors[indices[i]];
            if (distance[neighbor.IndexRsu(size)] > dist_ &&
                distance[neighbor.IndexLsd(size)] > dist_)
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
    
    while (currentSize < size_)
    {
        std::vector<Tile> coast;
        for (auto t : coverage)
        {
            coast.push_back(t);
        }
        
        int updatedSize = currentSize;

        for (auto t : coast)
        {
            if (updatedSize >= size_)
            {
                break;
            }
            
            auto neighbors = t.Neighbors(size);
            for (auto neighbor : neighbors)
            {
                if (updatedSize >= size_)
                {
                    break;
                }
                
                if (distance[neighbor.IndexRsu(size)] > dist_ &&
                    distance[neighbor.IndexLsd(size)] > dist_)
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
    
    placer_.Place(coverage, map, seaLevel);

    return TileConvert::ToPosition(coverage);
}
