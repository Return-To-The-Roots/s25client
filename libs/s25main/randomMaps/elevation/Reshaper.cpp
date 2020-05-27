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
#include "randomMaps/elevation/Reshaper.h"
#include "randomMaps/algorithm/GridUtility.h"
#include "randomMaps/algorithm/Filter.h"

#include <cmath>

unsigned char Reshaper::Scale(double alpha, double scale)
{
    auto x = scale <= 0.0 ? 1.0 - pow(alpha, -scale) : pow(alpha, scale);
    auto d = height_.maximum - height_.minimum;
    
    return char(round(d * rnd_.DRand(0.0, x)));
}

void Reshaper::ElevateCenter(std::vector<unsigned char>& heightMap,
                             const MapExtent& size, double scale)
{
    double max = GridUtility::Distance(Position(0,0), Position(size/2), size);
    double alpha;
    
    for (int x = 0; x < size.x; ++x)
    {
        for (int y = 0; y < size.y; ++y)
        {
            alpha = 1.0 - GridUtility::Distance(Position(x,y), Position(size/2), size) / max;
            heightMap[x + y * size.x] += Scale(alpha, scale);
        }
    }
}

void Reshaper::ElevateEdges(std::vector<unsigned char>& heightMap,
                            const MapExtent& size, double scale)
{
    double alpha;
    int dx, dy;

    for (int x = 0; x < size.x; ++x)
    {
        for (int y = 0; y < size.y; ++y)
        {
            dx = min(x, size.x-x-1);
            dy = min(y, size.y-y-1);
            
            if (dx < dy)
            {
                alpha = 1.0 - 2 * (double)dx / (double)size.x;
            }
            else
            {
                alpha = 1.0 - 2 * (double)dy / (double)size.y;
            }
            
            heightMap[x + y * size.x] += Scale(alpha, scale);
        }
    }
}

void Reshaper::ElevateCorners(std::vector<unsigned char>& heightMap,
                              const MapExtent& size, double scale)
{
    double max = GridUtility::Distance(Position(0,0), Position(size/2), size);
    double alpha;
    
    for (int x = 0; x < size.x; ++x)
    {
        for (int y = 0; y < size.y; ++y)
        {
            alpha = GridUtility::Distance(Position(x,y), Position(size/2), size) / max;
            heightMap[x + y * size.x] += Scale(alpha, scale);
        }
    }
}

void Reshaper::ElevateContrast(std::vector<unsigned char>& heightMap,
                               const MapExtent& size, double scale)
{
    int n = size.x * size.y;
    double maximum = 0.0;
    double mean = 0.0;
    for (int i = 0; i < n; ++i)
    {
        mean += heightMap[i];
        maximum = max(maximum, double(heightMap[i]));
    }
    mean /= n;

    double alpha;
    for (int x = 0; x < size.x; ++x)
    {
        for (int y = 0; y < size.y; ++y)
        {
            alpha = 1.0 - abs(heightMap[x + y * size.x] - mean) / maximum;
            heightMap[x + y * size.x] += Scale(alpha, scale);
        }
    }
}

void Reshaper::ElevateRandom(std::vector<unsigned char>& heightMap,
                             const MapExtent& size, double scale)
{
    for (int x = 0; x < size.x; ++x)
    {
        for (int y = 0; y < size.y; ++y)
        {
            heightMap[x + y * size.x] += Scale(0.5, scale);
        }
    }
}

void Reshaper::Elevate(std::vector<unsigned char>& heightMap,
                       ElevationMode mode,
                       const MapExtent& size,
                       double scale)
{
    switch (mode) {
        case Center:
            ElevateCenter(heightMap, size, scale);
            break;
        
        case Edges:
            ElevateEdges(heightMap, size, scale);
            break;
        
        case Corners:
            ElevateCorners(heightMap, size, scale);
            break;
            
        case Contrast:
            ElevateContrast(heightMap, size, scale);
            break;
            
        case Random:
            ElevateRandom(heightMap, size, scale);
            break;
            
        default:
            break;
    }
}

double Reshaper::ScaleCorner(int x, int y, ElevationCorner corner, const MapExtent& size)
{
    Position p1;
    Position p2;
    
    switch (corner)
    {
        case North:
            p1 = Position(0, size.y / 4);
            p2 = Position(0, y);
            break;
        case NorthEast:
            p1 = Position(3 * size.x / 4, size.y / 4);
            p2 = Position(x, y);
            break;
        case NorthWest:
            p1 = Position(size.x / 4, size.y / 4);
            p2 = Position(x, y);
            break;
        case West:
            p1 = Position(size.x / 4, 0);
            p2 = Position(x, 0);
            break;
        case East:
            p1 = Position(3 * size.x / 4, 0);
            p2 = Position(x, 0);
            break;
        case South:
            p1 = Position(0, 3 * size.y / 4);
            p2 = Position(0, y);
            break;
        case SouthWest:
            p1 = Position(size.x / 4, 3 * size.y / 4);
            p2 = Position(x, y);
            break;
        case SouthEast:
            p1 = Position(3 * size.x / 4, 3 * size.y / 4);
            p2 = Position(x, y);
            break;
        case Central:
            p1 = Position(size.x / 2, size.y / 2);
            p2 = Position(x, y);
            break;
    }
    
    return GridUtility::Distance(p1, p2, size);
}

void Reshaper::Elevate(std::vector<unsigned char>& z, ElevationCorner corner, double scale, const MapExtent& size)
{
    double maximum = GridUtility::Distance(Position(0, 0),
                                           Position(size.x / 2, size.y / 2),
                                           size);
    
    for (int x = 0; x < size.x; ++x)
    {
        for (int y = 0; y < size.y; ++y)
        {
            z[x + y * size.x] += Scale(ScaleCorner(x, y, corner, size) / maximum, scale);
        }
    }
}
