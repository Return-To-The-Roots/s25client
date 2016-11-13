// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include <cmath>
#include "mapGenerator/VertexUtility.h"

Vec2 VertexUtility::GetPosition(const int index, const int width, const int height)
{
    return Vec2(index % width, index / width);
}
    
int VertexUtility::GetIndexOf(const Vec2& pos, const int width, const int height)
{
    return VertexUtility::GetIndexOf(pos.x, pos.y, width, height);
}

int VertexUtility::GetIndexOf(const int x, const int y, const int width, const int height)
{
    return (x & (width - 1)) + (y & (height - 1)) * width;
}

std::vector<int> VertexUtility::GetNeighbors(const Vec2& pos, int width, int height, float radius)
{
    return VertexUtility::GetNeighbors(pos.x, pos.y, width, height, radius);
}

std::vector<int> VertexUtility::GetNeighbors(const int x,
                                             const int y,
                                             const int width,
                                             const int height,
                                             const float radius)
{
    std::vector<int> neighbors;
    const int r = (int)radius;
    
    for (int nx = x - r; nx <= x + r; nx++)
    {
        for (int ny = y - r; ny <= y + r; ny++)
        {
            double dist = std::sqrt((nx - x) * (nx - x) + (ny - y) * (ny - y));
            
            if (dist <= radius)
            {
                neighbors.push_back(VertexUtility::GetIndexOf(nx, ny, width, height));
            }
        }
    }
    
    return neighbors;
}

bool VertexUtility::IsInDistanceOf(const Vec2& pos1, const Vec2& pos2, const float distance)
{
    return Distance(pos1, pos2) < distance;
}

double VertexUtility::Distance(const Vec2& v1, const Vec2& v2)
{
    return std::sqrt((v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y));
}




