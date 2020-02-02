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

#include "mapGenerator/Tile.h"
#include "mapGenerator/GridUtility.h"

namespace rttr {
namespace mapGenerator {

bool Tile::IsValid(const MapExtent& size) const
{
    if (!rsu.isValid() || !lsd.isValid())
    {
        return false;
    }
    
    if (rsu.x < 0 || lsd.x < 0 || rsu.y < 0 || lsd.y < 0)
    {
        return false;
    }
    
    // ToDo: check if this computation is
    // 1) correct and
    // 2) cannot be simplified
    
    auto delta = rsu - lsd;
    
    if (delta.x > size.x / 2)
        delta.x = size.x - delta.x;
    
    if (delta.x < -size.x / 2)
        delta.x = size.x + delta.x;
    
    if (delta.y > size.y / 2)
        delta.y = size.y - delta.y;

    if (delta.y < -size.y / 2)
        delta.y = size.y + delta.y;
    
    return (delta.x == 0 && delta.y == 0)
        || (delta.x == 1 && delta.y == 0)
        || (rsu.y % 2 == 0 && delta.x == 1 && delta.y == 0)
        || (rsu.y % 2 == 0 && delta.x == 1 && delta.y == -1)
        || (rsu.y % 2 != 0 && delta.x == 0 && delta.y == -1)
        || (rsu.y % 2 != 0 && delta.x == 0 && delta.y == 1 && rsu.y == size.y - 1);
}

int Tile::IndexRsu(const MapExtent& size) const
{
    return rsu.x + rsu.y * size.x;
}

int Tile::IndexLsd(const MapExtent& size) const
{
    return lsd.x + lsd.y * size.x;
}

Tile Tile::Next(int x1, int y1, int x2, int y2, const MapExtent& size) const
{
    return Tile(GridClamp(Position(rsu.x + x1, rsu.y + y1), size),
                GridClamp(Position(lsd.x + x2, lsd.y + y2), size));
}
std::vector<Tile> Tile::Next(std::vector<int> o, const MapExtent& size) const
{
    return {
        Next(o[0],  o[1],  o[2],  o[3],  size),
        Next(o[4],  o[5],  o[6],  o[7],  size),
        Next(o[8],  o[9],  o[10], o[11], size),
        Next(o[12], o[13], o[14], o[15], size),
        Next(o[16], o[17], o[18], o[19], size),
        Next(o[20], o[21], o[22], o[23], size),
        Next(o[24], o[25], o[26], o[27], size),
        Next(o[28], o[29], o[30], o[31], size)
    };
}

std::vector<Tile> Tile::Neighbors(const MapExtent& size) const
{
    if (IsValid(size))
    {
        if (rsu.x == lsd.x && rsu.y == lsd.y && rsu.y % 2 == 0)
        {
            return Next({1, 0, 1, 0, 1, 0, 0, 1, 0, 1,-1, 1,-1, 1,-1, 1,-1, 0,-1, 0,-1,-1,-1, 0, 0,-1,-1,-1, 0,-1, 0,-1}, size);
        }
        
        if (rsu.x == lsd.x && rsu.y == lsd.y && rsu.y % 2 == 1)
        {
            return Next({1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1,-1, 0,-1, 0, 0,-1,-1, 0, 1,-1, 0,-1, 1,-1, 1,-1}, size);
        }
        
        if (rsu.x == lsd.x && rsu.y != lsd.y && rsu.y % 2 == 1)
        {
            return Next({1, 0, 0,-1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1,-1, 1, 0, 1,-1, 0,-1, 0,-1,-1, 0,-1,-1,-1, 1,-1, 0,-1}, size);
        }
        
        if (rsu.x != lsd.x && rsu.y == lsd.y && rsu.y % 2 == 0)
        {
            return Next({1, 0, 1, 0, 0, 1, 0, 1,-1, 1, 0, 1,-1, 0,-1, 1,-1, 0,-1, 0,-1,-1,-1,-1,-1,-1, 0,-1, 0,-1, 1, 0}, size);
        }
        
        if (rsu.x != lsd.x && rsu.y == lsd.y && rsu.y % 2 == 1)
        {
            return Next({1, 0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1,-1, 0, 0, 1,-1, 0,-1, 0, 0,-1, 0,-1, 0,-1, 1,-1, 1,-1, 1, 0}, size);
        }
        
        if (rsu.x != lsd.x && rsu.y != lsd.y && rsu.y % 2 == 0)
        {
            return Next({1, 0, 1,-1, 0, 1, 1, 0, 0, 1, 1, 1,-1, 1, 0, 1,-1, 1,-1, 0,-1, 0, 0,-1,-1,-1, 0,-1, 0,-1, 1,-1}, size);
        }
        
        if (rsu.x != lsd.x && rsu.y != lsd.y && rsu.y % 2 == 1)
        {
            return Next({1, 0, 0,-1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 1,-1, 1, 0, 1,-1, 0,-1, 0,-1,-1, 0,-1,-1,-1, 1,-1, 0,-1}, size);
        }
    }
    
    return {};
}

std::vector<Position> TileSetToPositions(const std::set<Tile, TileCompare>& tiles)
{
    std::set<Position, LessByPosition> positions;
    
    for (auto tile : tiles)
    {
        positions.insert(tile.lsd);
        positions.insert(tile.rsu);
    }
    
    return std::vector<Position>(positions.begin(), positions.end());
}

}}
