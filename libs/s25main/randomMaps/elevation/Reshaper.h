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

#ifndef Reshaper_h__
#define Reshaper_h__

#include "randomMaps/elevation/HeightSettings.h"
#include "randomMaps/algorithm/GridPredicate.h"
#include "randomMaps/algorithm/RandomUtility.h"
#include "gameTypes/MapCoordinates.h"

enum ElevationMode
{
    Center,
    Edges,
    Corners,
    Contrast,
    Random
};

enum ElevationCorner
{
    North = 0,
    NorthWest,
    NorthEast,
    West,
    South,
    SouthWest,
    SouthEast,
    East,
    Central
};

class Reshaper
{
private:
    RandomUtility rnd_;
    HeightSettings height_;
    
    unsigned char Scale(double alpha, double scale);
    double ScaleCorner(int x, int y, ElevationCorner corner, const MapExtent& size);

    void ElevateCenter(std::vector<unsigned char>& heightMap,
                       const MapExtent& size, double scale);
    void ElevateEdges(std::vector<unsigned char>& heightMap,
                      const MapExtent& size, double scale);
    void ElevateCorners(std::vector<unsigned char>& heightMap,
                        const MapExtent& size, double scale);
    void ElevateContrast(std::vector<unsigned char>& heightMap,
                         const MapExtent& size, double scale);
    void ElevateRandom(std::vector<unsigned char>& heightMap,
                       const MapExtent& size, double scale);
    
public:
    Reshaper(RandomUtility rnd, HeightSettings height)
        : rnd_(rnd), height_(height) {}
    
    void Elevate(std::vector<unsigned char>& heightMap,
                 ElevationMode mode,
                 const MapExtent& size,
                 double scale);
    
    void Elevate(std::vector<unsigned char>& z,
                 ElevationCorner corner,
                 double scale,
                 const MapExtent& size);
};

#endif // Reshaper_h__
