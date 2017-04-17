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

#include "mapGenerator/VertexUtility.h"
#include <cmath>

Point<uint16_t> VertexUtility::GetPosition(int index, int width, int height)
{
    return Point<uint16_t>(index % width, index / width);
}

int VertexUtility::GetIndexOf(int x, int y, int width, int height)
{
    return (x & (width - 1)) + (y & (height - 1)) * width;
}

std::vector<int> VertexUtility::GetNeighbors(int x,
                                             int y,
                                             int width,
                                             int height,
                                             int radius)
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

double VertexUtility::Distance(int x1,
                               int y1,
                               int x2,
                               int y2,
                               int width,
                               int height)
{
    int minX = std::min(x1, x2);
    int minY = std::min(y1, y2);
    int maxX = std::max(x1, x2);
    int maxY = std::max(y1, y2);
    
    int dx = (maxX - minX);
    int dy = (maxY - minY);
    
    if (dx > width / 2)  dx = width - dx;
    if (dy > height / 2) dy = height - dy;
    
    return std::sqrt(dx * dx + dy * dy);
}




