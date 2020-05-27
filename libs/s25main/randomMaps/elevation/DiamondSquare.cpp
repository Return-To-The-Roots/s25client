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
#include "randomMaps/elevation/DiamondSquare.h"
#include <cmath>

int DiamondSquare::NextPowerOfTwo(int number)
{
    number--;
    number |= number >> 1;
    number |= number >> 2;
    number |= number >> 4;
    number |= number >> 8;
    number |= number >> 16;
    number++;
    return number;
}

std::vector<unsigned char> DiamondSquare::Create(const MapExtent& size)
{
    // diamond square requres a squared map of the width 2^n+1
    // therefore we calcuate an extended map size
    int extendedSize = max(NextPowerOfTwo(size.y), NextPowerOfTwo(size.x)) + 1;
    std::vector<unsigned char> extendedMap(extendedSize * extendedSize, height_.minimum);

    // initialize corner values for diamond square
    extendedMap[0] = rnd_.Rand(height_.minimum, height_.maximum);
    extendedMap[extendedSize - 1] = rnd_.Rand(height_.minimum, height_.maximum);
    extendedMap[extendedSize * (extendedSize - 1)] = rnd_.Rand(height_.minimum, height_.maximum);
    extendedMap[extendedSize * extendedSize - 1] = rnd_.Rand(height_.minimum, height_.maximum);

    // apply actual diamond square algorithm
    ExecuteDS(extendedMap, MapExtent(extendedSize, extendedSize), extendedSize / 2);
    
    // transfer extended map to actual map size
    std::vector<unsigned char> heightMap(size.x * size.y, height_.minimum);
    
    for (int x = 0; x < size.x; ++x)
    {
        for (int y = 0; y < size.y; ++y)
        {
            heightMap[x + y * size.x] = extendedMap[x + y * (size.x + 1)];
        }
    }
    
    return heightMap;
}

void DiamondSquare::ExecuteDS(std::vector<unsigned char>& points, const MapExtent& size, int step)
{
    int half = step / 2;
    if (half < 1) return;
    
    // square steps
    for (int y = half; y < size.y; y+=step)
        for (int x = half; x < size.x; x+=step)
            SquareStep(points, x % size.x, y % size.y, size, half);
    
    // diamond steps
    int col = 0;
    for (int x = 0; x < size.x; x += half)
    {
        col++;
        //If this is an odd column.
        if (col % 2 == 1)
            for (int y = half; y < size.y; y += step)
                DiamondStep(points, x % size.x, y % size.y, size, half);
        else
            for (int y = 0; y < size.y; y += step)
                DiamondStep(points, x % size.x, y % size.y, size, half);
    }
    ExecuteDS(points, size, step / 2);
}

void DiamondSquare::SquareStep(std::vector<unsigned char>& points,
                               int x, int y, const MapExtent& size, int reach)
{
    int count = 0;
    unsigned char avg = 0;
    if (x - reach >= 0 && y - reach >= 0)
    {
        avg += points[x-reach + (y-reach) * size.x];
        count++;
    }
    if (x - reach >= 0 && y + reach < size.y)
    {
        avg += points[x-reach + (y+reach) * size.x];
        count++;
    }
    if (x + reach < size.x && y - reach >= 0)
    {
        avg += points[x+reach + (y-reach) * size.x];
        count++;
    }
    if (x + reach < size.x && y + reach < size.y)
    {
        avg += points[x+reach + (y+reach) * size.x];
        count++;
    }
    double range = double(reach) / size.x;
    avg += (height_.maximum - height_.minimum) * rnd_.DRand(-range, range);
    points[x + y * size.x] = char(round(avg / count));
}

void DiamondSquare::DiamondStep(std::vector<unsigned char>& points,
                                int x, int y, const MapExtent& size, int reach)
{
    int count = 0;
    unsigned char avg = 0;
    if (x - reach >= 0)
    {
        avg += points[x-reach + y * size.x];
        count++;
    }
    if (x + reach < size.x)
    {
        avg += points[x+reach + y * size.x];
        count++;
    }
    if (y - reach >= 0)
    {
        avg += points[x + (y-reach) * size.x];
        count++;
    }
    if (y + reach < size.y)
    {
        avg += points[x + (y+reach) * size.x];
        count++;
    }
    double range = double(reach) / size.x;
    avg += (height_.maximum - height_.minimum) * rnd_.DRand(-range, range);
    points[x + y * size.x] = char(round(avg / count));
}
