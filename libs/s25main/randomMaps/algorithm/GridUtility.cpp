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
#include "randomMaps/algorithm/GridUtility.h"
#include <cmath>
#include <queue>
#include <set>

Position GridUtility::GetPosition(int index, const MapExtent& size)
{
    return Position(index % size.x, index / size.x);
}

Position GridUtility::Clamp(const Position& p,
                            const MapExtent& size)
{
    return Position(p.x < 0 ? (size.x - (-p.x) % size.x) % size.x : p.x % size.x,
                    p.y < 0 ? (size.y - (-p.y) % size.y) % size.y : p.y % size.y);
}

Position GridUtility::Delta(const Position& p1, const Position& p2, const MapExtent& size)
{
    auto delta = p1 - p2;
    
    if (delta.x > size.x / 2)
        delta.x = size.x - delta.x;
    
    if (delta.x < -size.x / 2)
        delta.x = size.x + delta.x;
    
    if (delta.y > size.y / 2)
        delta.y = size.y - delta.y;

    if (delta.y < -size.y / 2)
        delta.y = size.y + delta.y;

    return delta;
}

double GridUtility::Distance(const Position& p1,
                             const Position& p2,
                             const MapExtent& size)
{
    Position minPos = elMin(p1, p2);
    Position maxPos = elMax(p1, p2);
    Position delta = maxPos - minPos;
    
    if(delta.x > size.x / 2)
        delta.x = size.x - delta.x;
    if(delta.y > size.y / 2)
        delta.y = size.y - delta.y;
    
    Position deltaSq = delta * delta;
    return std::sqrt(deltaSq.x + deltaSq.y);
}

double GridUtility::DistanceNorm(const Position& p1, const Position& p2, const MapExtent& size)
{
    auto mapEdge = Position(0, 0);
    auto mapCenter = Position(size.x/2, size.y/2);

    auto maximumDistance = Distance(mapEdge, mapCenter, size);
    auto actualDistance = Distance(p1, p2, size);
    
    return min(1.0, actualDistance / maximumDistance);
}

std::vector<Position> GridUtility::Positions(const MapExtent& size)
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

std::vector<Position> GridUtility::Neighbors(const Position& p, const MapExtent& size)
{
    return {
        Clamp(Position(p.x + 1, p.y), size),
        Clamp(Position(p.x, p.y + 1), size),
        Clamp(Position(p.x - 1, p.y), size),
        Clamp(Position(p.x, p.y - 1), size)
    };
}

std::vector<Position> GridUtility::Collect(const Position& p, const MapExtent& size, double distance)
{
    std::vector<Position> result;
    int d = int(distance) + 1;
    
    for (int x = p.x - d; x < p.x + d; x++)
    {
        for (int y = p.y - d; y < p.y + d; y++)
        {
            auto pos = Clamp(Position(x,y), size);
            
            if (Distance(pos, p, size) < distance)
            {
                result.push_back(pos);
            }
        }
    }
    
    return result;
}

std::vector<Position> GridUtility::Collect(const Position& p,
                                           const MapExtent& size,
                                           const std::vector<bool>& property)
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
        if (property[index] == propertyValue && indices.find(index) == indices.end())
        {
            indices.insert(index);
            body.push_back(pos);
            
            // push neighbors into the queue
            searchSpace.push(Clamp(Position(pos.x + 1, pos.y), size));
            searchSpace.push(Clamp(Position(pos.x, pos.y + 1), size));
            searchSpace.push(Clamp(Position(pos.x - 1, pos.y), size));
            searchSpace.push(Clamp(Position(pos.x, pos.y - 1), size));
        }
    }

    return body;
}

std::vector<Position> GridUtility::NeighborsOfRsuTriangle(const Position& p,
                                                          const MapExtent& size)
{
    if (p.y % 2 == 0)
    {
        return {
            Position(p),
            Clamp(Position(p.x-1, p.y), size),
            Clamp(Position(p.x-1, p.y+1), size)
        };
    }
    
    return {
        Position(p),
        Clamp(Position(p.x-1, p.y), size),
        Clamp(Position(p.x, p.y+1), size)
    };
}

std::vector<Position> GridUtility::NeighborsOfLsdTriangle(const Position& p,
                                                          const MapExtent& size)
{
    if (p.y % 2 == 0 && p.y != 0)
    {
        return {
            Position(p),
            Clamp(Position(p.x, p.y-1), size),
            Clamp(Position(p.x+1, p.y), size)
        };
    }
    
    return {
        Position(p),
        Clamp(Position(p.x+1, p.y), size),
        Clamp(Position(p.x+1, p.y-1), size)
    };
}

