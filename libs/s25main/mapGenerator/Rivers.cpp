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

#include "mapGenerator/Rivers.h"

namespace rttr {
namespace mapGenerator {

RiverBrush::RiverBrush(TextureMapping_& mapping) : mapping_(mapping)
{
    
}

void RiverBrush::Paint(std::vector<Tile>& river, Map_& map)
{
    const int sea = map.sea;
    const Texture waterTexture = mapping_.water;
    const Texture coast1Texture = mapping_.coast;
    const Texture coast2Texture = mapping_.GetCoastTerrain(1);
    const Texture coast3Texture = mapping_.GetCoastTerrain(2);

    const auto size = map.size;
    
    std::set<int> rsu;
    std::set<int> lsd;
    
    for (auto const& tile : river)
    {
        int indexRsu = tile.IndexRsu(size);
        int indexLsd = tile.IndexLsd(size);
        
        rsu.insert(indexRsu);
        lsd.insert(indexLsd);

        map.z[indexRsu] = std::max(static_cast<int>(sea), map.z[indexRsu] - 1);
        map.z[indexLsd] = std::max(static_cast<int>(sea), map.z[indexLsd] - 1);
        
        map.textureRsu[indexRsu] = waterTexture;
        map.textureLsd[indexLsd] = waterTexture;
    }

    std::vector<Tile> coast;
    for (auto const& tile : river)
    {
        auto neighbors = tile.Neighbors(size);
        for (auto neighbor : neighbors)
        {
            coast.push_back(neighbor);
            
            int indexRsu = neighbor.IndexRsu(size);
            int indexLsd = neighbor.IndexLsd(size);
            
            if (!mapping_.IsMountain(map.textureRsu[indexRsu]) &&
                map.textureRsu[indexRsu] != waterTexture)
            {
                map.textureRsu[indexRsu] = coast1Texture;
            }

            if (!mapping_.IsMountain(map.textureLsd[indexLsd]) &&
                map.textureLsd[indexLsd] != waterTexture)
            {
                map.textureLsd[indexLsd] = coast1Texture;
            }
        }
    }
    
    std::vector<Tile> transition;
    for (auto const& tile : coast)
    {
        auto neighbors = tile.Neighbors(size);
        for (auto neighbor : neighbors)
        {
            transition.push_back(neighbor);

            int indexRsu = neighbor.IndexRsu(size);
            int indexLsd = neighbor.IndexLsd(size);
            
            if (!mapping_.IsMountain(map.textureRsu[indexRsu]) &&
                map.textureRsu[indexRsu] != waterTexture &&
                map.textureRsu[indexRsu] != coast1Texture)
            {
                map.textureRsu[indexRsu] = coast2Texture;
            }

            if (!mapping_.IsMountain(map.textureLsd[indexLsd]) &&
                map.textureLsd[indexLsd] != waterTexture &&
                map.textureLsd[indexLsd] != coast1Texture)
            {
                map.textureLsd[indexLsd] = coast2Texture;
            }
        }
    }
    
    for (auto const& tile : transition)
    {
        auto neighbors = tile.Neighbors(size);
        for (auto const& neighbor : neighbors)
        {
            int indexRsu = neighbor.IndexRsu(size);
            int indexLsd = neighbor.IndexLsd(size);
            
            if (!mapping_.IsMountain(map.textureRsu[indexRsu]) &&
                map.textureRsu[indexRsu] != waterTexture &&
                map.textureRsu[indexRsu] != coast1Texture &&
                map.textureRsu[indexRsu] != coast2Texture)
            {
                map.textureRsu[indexRsu] = coast3Texture;
            }

            if (!mapping_.IsMountain(map.textureLsd[indexLsd]) &&
                map.textureLsd[indexLsd] != waterTexture &&
                map.textureLsd[indexLsd] != coast1Texture &&
                map.textureLsd[indexLsd] != coast2Texture)
            {
                map.textureLsd[indexLsd] = coast3Texture;
            }
        }
    }
}

Tile NextRiverTile(RandomUtility& rnd, Tile& tile, int direction, const MapExtent& size)
{
    auto neighbors = tile.Neighbors(size);
    auto index = direction + rnd.Rand(-1, 1);
    
    return index < 0 ? neighbors[7] : neighbors[index % 8];
}

River::River(RiverBrush brush, int direction, Tile source)
    : brush_(brush), direction_(direction), location_(source)
{
    
}

River& River::ExtendBy(RandomUtility& rnd, int length, const MapExtent& size)
{
    if (length == 0)
    {
        return *this;
    }

    coverage_.push_back(location_);
    location_ = NextRiverTile(rnd, location_, direction_, size);

    for (auto& stream : streams_)
    {
        stream.ExtendBy(rnd, 1, size);
    }
    
    return ExtendBy(rnd, length - 1, size);
}

River& River::Steer(bool clockwise, bool swap)
{
    auto direction = clockwise ? (direction_ + 1) % 8 : direction_ - 1;
    direction_ = direction < 0 ? 7 : direction;
    
    for (auto& stream : streams_)
    {
        stream.Steer(swap ? !clockwise : clockwise, swap);
    }

    return *this;
}

River& River::Split(bool clockwise, bool recursive)
{
    if (recursive)
    {
        for (auto& stream : streams_)
        {
            stream.Split(clockwise, recursive);
        }
    }

    auto direction = clockwise ? (direction_ + 1) % 8 : direction_ - 1;
    auto river = River(brush_, direction < 0 ? 7 : direction, location_);
    
    streams_.push_back(river);

    return *this;
}

void River::Create(Map_& map)
{
    brush_.Paint(coverage_, map);
    
    for (auto& stream : streams_)
    {
        stream.Create(map);
    }
}

RandomRiver::RandomRiver(RiverBrush brush) : brush_(brush)
{
    
}

void RandomRiver::Create(RandomUtility& rnd, Map_& map, Tile source, int direction, int length)
{
    MapExtent size(map.size);
    River river(brush_, direction, source);
    
    // 10 times chance of split
    int splitIndex = length / 10;
    
    // 5 times chance of changing direction
    int steerIndex = length / 5;
    
    for (int i = 1; i <= length; i++)
    {
        if (splitIndex > 1 && i % splitIndex == 0)
        {
            // 20% chance to actually perform a split of the river
            if (rnd.ByChance(20))
            {
                // 50% chance that the new river stream goes into
                // clockwise direction compared to the current river's
                // direction. Otherwise counter-clockwise.
                bool clockwise = rnd.ByChance(50);

                // 10% chance to recursively split all streams of the
                // river, not just the main river.
                bool recursive = rnd.ByChance(10);
                
                river.Split(clockwise, recursive);
            }
        }
        
        if (steerIndex > 1 && i % steerIndex == 0)
        {
            // 20% chance to change direction of the river
            if (rnd.ByChance(20))
            {
                // 50% chance to change direction clockwise otherwise
                // into counter-clockwise direction.
                bool clockwise = rnd.ByChance(50);
                
                // 50% chance to swap direction of child streams of the river
                bool swap = rnd.ByChance(50);
                
                river.Steer(clockwise, swap);
            }
        }
        
        river.ExtendBy(rnd, 1, size);
    }
    
    river.Create(map);
}

}}
