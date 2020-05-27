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

#ifndef DiamondSquare_h__
#define DiamondSquare_h__

#include "randomMaps/elevation/HeightMap.h"
#include <vector>

class DiamondSquare : public HeightMap
{
private:
    int NextPowerOfTwo(int number);
    
    void SquareStep(std::vector<unsigned char>& points,
                    int x, int y, const MapExtent& size, int reach);
    void DiamondStep(std::vector<unsigned char>& points,
                     int x, int y, const MapExtent& size, int reach);
    void ExecuteDS(std::vector<unsigned char>& points,
                   const MapExtent& size, int step);
    
public:
    DiamondSquare(const RandomUtility& rnd,
                  const HeightSettings& height) : HeightMap(rnd, height) {}
    
    std::vector<unsigned char> Create(const MapExtent& size);
};

#endif // DiamondSquare_h__
