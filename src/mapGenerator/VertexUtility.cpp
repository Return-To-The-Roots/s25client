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

#include "defines.h" // IWYU pragma: keep
#include "mapGenerator/VertexUtility.h"
#include <cmath>
#include <algorithm>

Point<uint16_t> VertexUtility::GetPosition(int index, int width, int height)
{
    return Point<uint16_t>(index % width, index / width);
}

int VertexUtility::GetIndexOf(const Point<int>& p, int width, int height)
{
    return (p.x & (width - 1)) + (p.y & (height - 1)) * width;
}

std::vector<int> VertexUtility::GetNeighbors(const Point<int>& p,
                                             int width,
                                             int height,
                                             int radius)
{
    std::vector<int> neighbors;

    for (int nx = p.x - radius; nx <= p.x + radius; nx++)
    {
        for (int ny = p.y - radius; ny <= p.y + radius; ny++)
        {
            const Point<int> neighbor(nx,ny);
            if (VertexUtility::Distance(p, neighbor, width, height) <= radius)
            {
                neighbors.push_back(VertexUtility::GetIndexOf(neighbor, width, height));
            }
        }
    }
    
    return neighbors;
}

double VertexUtility::Distance(const Point<int>& p1,
                               const Point<int>& p2,
                               int width,
                               int height)
{
    int minX = std::min(p1.x, p2.x);
    int minY = std::min(p1.y, p2.y);
    int maxX = std::max(p1.x, p2.x);
    int maxY = std::max(p1.y, p2.y);
    
    int dx = (maxX - minX);
    int dy = (maxY - minY);
    
    if (dx > width / 2)  dx = width - dx;
    if (dy > height / 2) dy = height - dy;
    
    return std::sqrt(dx * dx + dy * dy);
}




