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

#include "rttrDefines.h"
#include "mapGenerator/Reshaper.h"
#include "mapGenerator/GridUtility.h"

#include <cmath>
#include <numeric>

namespace rttr {
namespace mapGenerator {

unsigned char Reshaper::Scale(double alpha, double scale)
{
    auto x = scale <= 0. ? 1. - pow(alpha, -scale) : pow(alpha, scale);
    auto d = height_.max - height_.min;
    
    return char(round(d * rnd_.DRand(0., x)));
}

void Reshaper::ElevateCenter(HeightMap& z, const MapExtent& size, double scale)
{
    const Position center(size/2);
    const Position zero(0,0);
    
    const double distance = GridDistance(zero, center, size);
    
    RTTR_FOREACH_PT(Position, size)
    {
        z[pt.x + pt.y * size.x] += Scale(1. - GridDistance(pt, center, size) / distance, scale);
    }
}

void Reshaper::ElevateEdges(HeightMap& z, const MapExtent& size, double scale)
{
    double alpha;
    int dx, dy;

    RTTR_FOREACH_PT(Position, size)
    {
        dx = std::min(pt.x, size.x-pt.x-1);
        dy = std::min(pt.y, size.y-pt.y-1);
        
        if (dx < dy)
        {
            alpha = 1. - 2 * static_cast<double>(dx) / static_cast<double>(size.x);
        }
        else
        {
            alpha = 1. - 2 * static_cast<double>(dy) / static_cast<double>(size.y);
        }
        
        z[pt.x + pt.y * size.x] += Scale(alpha, scale);
    }
}

void Reshaper::ElevateCorners(HeightMap& z, const MapExtent& size, double scale)
{
    const Position zero(0,0);
    const Position center(size/2);
    
    const double distance = GridDistance(zero, center, size);
    
    RTTR_FOREACH_PT(Position, size)
    {
        z[pt.x + pt.y * size.x] += Scale(GridDistance(pt, center, size) / distance, scale);
    }
}

void Reshaper::ElevateContrast(HeightMap& z, const MapExtent& size, double scale)
{
    const int nodes = size.x * size.y;

    const auto mean = std::accumulate(z.begin(), z.end(), 0u) / nodes;
    const auto maximum = *std::max_element(z.begin(), z.end());
    
    for (int i = 0; i < nodes; ++i)
    {
        z[i] += Scale(1. - abs(static_cast<int>(z[i] - mean)) /
                      static_cast<double>(maximum), scale);
    }
}

void Reshaper::ElevateRandom(HeightMap& z, const MapExtent& size, double scale)
{
    const int nodes = size.x * size.y;
    
    for (int i = 0; i < nodes; ++i)
    {
        z[i] += Scale(0.5, scale);
    }
}

void Reshaper::Elevate(HeightMap& z, Reshaper::Mode mode, const MapExtent& size, double scale)
{
    switch (mode)
    {
        case Center:
            ElevateCenter(z, size, scale);
            break;
        
        case Edges:
            ElevateEdges(z, size, scale);
            break;
        
        case Corners:
            ElevateCorners(z, size, scale);
            break;
            
        case Contrast:
            ElevateContrast(z, size, scale);
            break;
            
        case Random:
            ElevateRandom(z, size, scale);
            break;
    }
}

double Reshaper::ScaleCorner(const MapPoint& p, Reshaper::Corner corner, const MapExtent& size) const
{
    Position p1;
    Position p2;
    
    switch (corner)
    {
        case North:
            p1 = Position(0, size.y / 4);
            p2 = Position(0, p.y);
            break;
        case NorthEast:
            p1 = Position(3 * size.x / 4, size.y / 4);
            p2 = Position(p.x, p.y);
            break;
        case NorthWest:
            p1 = Position(size.x / 4, size.y / 4);
            p2 = Position(p.x, p.y);
            break;
        case West:
            p1 = Position(size.x / 4, 0);
            p2 = Position(p.x, 0);
            break;
        case East:
            p1 = Position(3 * size.x / 4, 0);
            p2 = Position(p.x, 0);
            break;
        case South:
            p1 = Position(0, 3 * size.y / 4);
            p2 = Position(0, p.y);
            break;
        case SouthWest:
            p1 = Position(size.x / 4, 3 * size.y / 4);
            p2 = Position(p.x, p.y);
            break;
        case SouthEast:
            p1 = Position(3 * size.x / 4, 3 * size.y / 4);
            p2 = Position(p.x, p.y);
            break;
        case Central:
            p1 = Position(size.x / 2, size.y / 2);
            p2 = Position(p.x, p.y);
            break;
    }
    
    return GridDistance(p1, p2, size);
}

void Reshaper::Elevate(HeightMap& z, Reshaper::Corner corner, double scale, const MapExtent& size)
{
    const Position center(size/2);
    const Position zero(0,0);

    const double distance = GridDistance(zero, center, size);
    
    RTTR_FOREACH_PT(MapPoint, size)
    {
        z[pt.x + pt.y * size.x] += Scale(ScaleCorner(pt, corner, size) / distance, scale);
    }
}

}}
