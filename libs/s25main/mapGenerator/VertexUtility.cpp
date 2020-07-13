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

#include "mapGenerator/VertexUtility.h"
#include <algorithm>
#include <cmath>

Position VertexUtility::GetPosition(int index, const MapExtent& size)
{
    return Position(index % size.x, index / size.x);
}

int VertexUtility::GetIndexOf(Position p, const MapExtent& size)
{
    p.x %= size.x;
    p.y %= size.y;
    if(p.x < 0)
        p.x += size.x;
    if(p.y < 0)
        p.y += size.y;
    return p.x + p.y * size.x;
}

std::vector<int> VertexUtility::GetNeighbors(const Position& p, const MapExtent& size, int radius)
{
    std::vector<int> neighbors;

    for(int nx = p.x - radius; nx <= p.x + radius; nx++)
    {
        for(int ny = p.y - radius; ny <= p.y + radius; ny++)
        {
            const Position neighbor(nx, ny);
            if(VertexUtility::Distance(p, neighbor, size) <= radius)
            {
                neighbors.push_back(VertexUtility::GetIndexOf(neighbor, size));
            }
        }
    }

    return neighbors;
}

double VertexUtility::Distance(const Position& p1, const Position& p2, const MapExtent& size)
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
