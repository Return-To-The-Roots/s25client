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

#include "mapGenerator/HeightMap.h"
#include "mapGenerator/RandomUtility.h"
#include "gameTypes/MapCoordinates.h"

namespace rttr {
namespace mapGenerator {

class Reshaper
{
public:
    
    enum Mode
    {
        Center,
        Edges,
        Corners,
        Contrast,
        Random
    };
    
    enum Corner
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
    
private:
    RandomUtility rnd_;
    Range height_;
    
    /**
     * Scales the specified value alpha by applying a function of the specified scale factor.
     * @param alpha value to scale
     * @param scale scale factor influencing the scaling procedure
     * @returns the scaled alpha value.
     */
    unsigned char Scale(double alpha, double scale);
    
    /**
     * Computes the scale factor to apply for elevation of the specified point. The scale factor depends on
     * the distance between the point and the choosen corner for elevation.
     * @param p point to compute the scale factor for
     * @param corner corner of the map chosen for elevation
     * @param size size of the map
     * @returns scale factor for elevation of the specified point (x, y).
     */
    double ScaleCorner(const MapPoint& p, Corner corner, const MapExtent& size) const;

    /**
     * Elevates the specified height map around the center of the map.
     * @param z height map to elevate
     * @param size size of the map
     * @param scale scale of elevation
     */
    void ElevateCenter(HeightMap& z, const MapExtent& size, double scale);
    
    /**
     * Elevates the specified height map around the edges of the map.
     * @param z height map to elevate
     * @param size size of the map
     * @param scale scale of elevation
     */
    void ElevateEdges(HeightMap& z, const MapExtent& size, double scale);
    
    /**
     * Elevates the specified height map around all four corners of the map.
     * @param z height map to elevate
     * @param size size of the map
     * @param scale scale of elevation
     */
    void ElevateCorners(HeightMap& z, const MapExtent& size, double scale);
    
    /**
     * Elevates the specified height map to sharpen height contrasts.
     * @param z height map to elevate
     * @param size size of the map
     * @param scale scale of elevation
     */
    void ElevateContrast(HeightMap& z, const MapExtent& size, double scale);

    /**
     * Randomly elevates the specified height map.
     * @param z height map to elevate
     * @param size size of the map
     * @param scale scale of elevation
     */
    void ElevateRandom(HeightMap& z, const MapExtent& size, double scale);
    
public:
    
    Reshaper(RandomUtility rnd, Range height) : rnd_(rnd), height_(height) {}
    
    /**
     * Elevates parts of the specified height map by applying the specified mode.
     * @param z height map to apply elevation to
     * @param mode mode to use for elevating landscape
     * @param size size of the map
     * @param scale scale of elevation to apply to the height map
     */
    void Elevate(HeightMap& z, Mode mode, const MapExtent& size, double scale);

    /**
     * Elevates the height map around the specified corner with the specified scale.
     * @param z height map to apply elevation to
     * @param corner corner of the map to elevate
     * @param scale scale of elevation to apply to the map
     * @param size size of the map
     */
    void Elevate(HeightMap& z, Corner corner, double scale, const MapExtent& size);
};

}}

#endif // Reshaper_h__
