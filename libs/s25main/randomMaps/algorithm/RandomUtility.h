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

#ifndef RandomUtility_h__
#define RandomUtility_h__

#include "gameTypes/MapCoordinates.h"
#include "random/XorShift.h"
#include <vector>

class RandomUtility
{
private:
    typedef XorShift UsedRNG;
    UsedRNG rng_;
    
public:
    RandomUtility();
    RandomUtility(uint64_t seed);
    
    bool ByChance(int percentage);
    
    int Index(const size_t& size);
    
    Position PRand(const MapExtent& size);
    
    int Rand(int min, int max);
    
    double DRand(double min, double max);
    
    std::vector<int> IRand(int n);
    
    void Shuffle(std::vector<Position>& area);
};

#endif // RandomUtility_h__

