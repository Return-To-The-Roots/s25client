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

int VertexUtility::GetIndexOf(const int x, const int y, const int width, const int height)
{
    return (x & (width - 1)) + (y & (height - 1)) * width;
}

std::vector<int> VertexUtility::GetNeighbors(const int x,
                                             const int y,
                                             const int width,
                                             const int height,
                                             const int radius)
{
    std::vector<int> neighbors;

    for (int nx = x - radius; nx <= x + radius; nx++)
    {
        for (int ny = y - radius; ny <= y + radius; ny++)
        {
            if (VertexUtility::Distance(x, y, nx, ny, width, height) <= radius)
            {
                neighbors.push_back(VertexUtility::GetIndexOf(nx, ny, width, height));
            }
        }
    }
    
    return neighbors;
}

double VertexUtility::Distance(const int x1,
                               const int y1,
                               const int x2,
                               const int y2,
                               const int width,
                               const int height)
{
    int min_x = x1 < x2 ? x1 : x2, min_y = y1 < y2 ? y1 : y2;
    int max_x = x1 > x2 ? x1 : x2, max_y = y1 > y2 ? y1 : y2;
    
    int dx = (max_x - min_x);
    int dy = (max_y - min_y);
    
    if (dx > width / 2)  dx = width - dx;
    if (dy > height / 2) dy = height - dy;
    
    return std::sqrt(dx * dx + dy * dy);
}




