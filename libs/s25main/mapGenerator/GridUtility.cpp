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

#include "mapGenerator/GridUtility.h"

#include <cmath>
#include <queue>
#include <set>

namespace rttr {
namespace mapGenerator {

Position GridPosition(int index, const MapExtent& size)
{
    return Position(index % size.x, index / size.x);
}

Position GridClamp(const Position& p, const MapExtent& size)
{
    return Position(p.x < 0 ? (size.x - (-p.x) % size.x) % size.x : p.x % size.x,
                    p.y < 0 ? (size.y - (-p.y) % size.y) % size.y : p.y % size.y);
}

Position GridDelta(const Position& p1, const Position& p2, const MapExtent& size)
{
    int dy = p2.y - p1.y;
    int dyi = dy < 0 ? size.y + dy : dy - size.y;
    
    dy = abs(dyi) < abs(dy) ? dyi : dy;
    
    if (dy > 0)
    {
        int dx = p2.x + (dy + p2.y % 2) / 2 - p1.x;
        int dxi = dx < 0 ? size.x + dx : dx - size.x;
        
        dx = abs(dxi) < abs(dx) ? dxi : dx;
        
        return Position(dx, dy);
    }
    else
    {
        int dx = p2.x + (dy - p1.y % 2) / 2 - p1.x;
        int dxi = dx < 0 ? size.x + dx : dx - size.x;
        
        dx = abs(dxi) < abs(dx) ? dxi : dx;
        
        return Position(dx, dy);
    }
}

double GridDistance(const Position& p1, const Position& p2, const MapExtent& size)
{
    Position delta = GridDelta(p1, p2, size);
    Position deltaSq = delta * delta;
    
    return std::sqrt(deltaSq.x + deltaSq.y);
}

double GridDistanceNorm(const Position& p1, const Position& p2, const MapExtent& size)
{
    auto mapEdge = Position(0, 0);
    auto mapCenter = Position(size.x/2, size.y/2);

    auto maximumDistance = GridDistance(mapEdge, mapCenter, size);
    auto actualDistance = GridDistance(p1, p2, size);
    
    return std::min(1.0, actualDistance / maximumDistance);
}

Positions GridPositions(const MapExtent& size)
{
    std::vector<Position> positions(size.x * size.y);
    for (int x = 0; x < size.x; x++)
    {
        for (int y = 0; y < size.y; y++)
        {
            positions[x + y * size.x] = Position(x,y);
        }
    }
    return positions;
}

Positions GridNeighbors(const Position& p, const MapExtent& size)
{
    if (p.y % 2 == 0)
    {
        return {
            GridClamp(Position(p.x + 1, p.y), size),
            GridClamp(Position(p.x - 1, p.y), size),
            GridClamp(Position(p.x - 1, p.y + 1), size),
            GridClamp(Position(p.x, p.y - 1), size)
        };
    }
    else
    {
        return {
            GridClamp(Position(p.x + 1, p.y), size),
            GridClamp(Position(p.x - 1, p.y), size),
            GridClamp(Position(p.x + 1, p.y - 1), size),
            GridClamp(Position(p.x, p.y + 1), size)
        };
    }
}

Positions GridCollect(const Position& p, const MapExtent& size, double distance)
{
    std::vector<Position> result;
    int d = int(distance) + 1;
    
    for (int x = p.x - d; x < p.x + d; x++)
    {
        for (int y = p.y - d; y < p.y + d; y++)
        {
            auto pos = GridClamp(Position(x,y), size);
            
            if (GridDistance(pos, p, size) < distance)
            {
                result.push_back(pos);
            }
        }
    }
    
    return result;
}

Positions GridCollect(const Position& p, const MapExtent& size, const std::vector<bool>& property)
{
    bool propertyValue = property[p.x + p.y * size.x];
    
    std::set<int> indices;
    std::vector<Position> body;
    std::queue<Position> searchSpace;
    
    searchSpace.push(p);
    
    // stop search if no further neighbors are available
    while(!searchSpace.empty())
    {
        // get and remove the last element from the queue
        Position pos = searchSpace.front();
        searchSpace.pop();

        // check if neighbor property matches initial point's property
        int index = pos.x + pos.y * size.x;
        if (property[index] == propertyValue)
        {
            if (indices.insert(index).second)
            {
                body.push_back(pos);
                
                // push neighbors into the queue
                auto neighbors = GridNeighbors(pos, size);
                for (Position neighbor : neighbors)
                {
                    searchSpace.push(neighbor);
                }
            }
        }
    }

    return body;
}

Positions GridNeighborsOfRsuTriangle(const Position& p, const MapExtent& size)
{
    if (p.y % 2 == 0)
    {
        return {
            Position(p),
            GridClamp(Position(p.x-1, p.y), size),
            GridClamp(Position(p.x-1, p.y+1), size)
        };
    }
    
    return {
        Position(p),
        GridClamp(Position(p.x-1, p.y), size),
        GridClamp(Position(p.x, p.y+1), size)
    };
}

Positions GridNeighborsOfLsdTriangle(const Position& p, const MapExtent& size)
{
    if (p.y % 2 == 0 && p.y != 0)
    {
        return {
            Position(p),
            GridClamp(Position(p.x, p.y-1), size),
            GridClamp(Position(p.x+1, p.y), size)
        };
    }
    
    return {
        Position(p),
        GridClamp(Position(p.x+1, p.y), size),
        GridClamp(Position(p.x+1, p.y-1), size)
    };
}

}}
