// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef HeightMap_h__
#define HeightMap_h__

#include "randomMaps/elevation/HeightSettings.h"
#include "randomMaps/algorithm/RandomUtility.h"
#include "gameTypes/MapCoordinates.h"
#include <vector>

class HeightMap
{
protected:
    RandomUtility rnd_;
    HeightSettings height_;
    
public:
    HeightMap(const RandomUtility& rnd,
              const HeightSettings& height) : rnd_(rnd), height_(height) {}
    
    virtual ~HeightMap() {}
    virtual std::vector<unsigned char> Create(const MapExtent& size)
    {
        return std::vector<unsigned char>(size.x * size.y, height_.minimum);
    }
};

class RandomHeightMap : public HeightMap
{
public:
    RandomHeightMap(const RandomUtility& rnd,
                    const HeightSettings& height) : HeightMap(rnd, height) {}
    
    std::vector<unsigned char> Create(const MapExtent& size)
    {
        std::vector<unsigned char> heightMap(size.x * size.y);
        for (int i = 0; i < size.x * size.y; ++i)
        {
            heightMap[i] = rnd_.Rand(height_.minimum, height_.maximum);
        }
        return heightMap;
    }
};

#endif // HeightMap_h__
