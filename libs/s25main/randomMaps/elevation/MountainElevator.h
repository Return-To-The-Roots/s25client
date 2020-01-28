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

#ifndef MountainElevator_h__
#define MountainElevator_h__

#include "randomMaps/elevation/Smoother.h"
#include "randomMaps/elevation/HeightSettings.h"
#include "randomMaps/algorithm/RandomUtility.h"
#include "gameTypes/MapCoordinates.h"

class MountainElevator
{
private:
    HeightSettings height_;
    
public:
    MountainElevator(HeightSettings height);
    
    void Increase(std::vector<unsigned char>& heightMap, unsigned char mountainLevel, const MapExtent& size);
};

#endif // MountainElevator_h__
