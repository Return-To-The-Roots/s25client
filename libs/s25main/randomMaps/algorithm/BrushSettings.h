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

#ifndef BrushSettings_h__
#define BrushSettings_h__

#include "Point.h"
#include <vector>

/**
 * BrushSettings are essentially describing the shape of brush.
 */
struct BrushSettings
{
    /**
     * Initializes the right-side-up ("rsu") and the left-side-down ("lsd") vectors with the specified vectors.
     * @param rsuArg values for copied to the "rsu" vector
     * @param lsdArg values for copied to the "lsd" vector
     */
    BrushSettings(std::vector<Position> rsuArg, std::vector<Position> lsdArg)
    {
        rsu = rsuArg;
        lsd = lsdArg;
    }
    
    /**
     * Initializes the right-side-up ("rsu") and the left-side-down ("lsd") vectors from the specified integer vectors.
     * The integer values of the vectors are being interpret as x-y-pairs like this: [ x0, y0, x1, y1, x2, y2, ... ]
     * @param rsuArg integer vector used to populate the x- and y-components of the "rsu" vector
     * @param lsdArg integer vector used to populate the x- and y-components of the "lsd" vector
     */
    BrushSettings(std::vector<int> rsuArg, std::vector<int> lsdArg)
    {
        for (auto i = 0u; i < rsuArg.size() / 2; i++)
        {
            rsu.push_back(Position(rsuArg[i*2], rsuArg[i*2+1]));
        }
        for (auto i = 0u; i < lsdArg.size() / 2; i++)
        {
            lsd.push_back(Position(lsdArg[i*2], lsdArg[i*2+1]));
        }
    }

    /**
     * Vector of right-side-up (RSU) positions used to apply the brush to. As the S2 grid is built up by triangles
     * pointing either upwards or downwards, this vector contains only positions of up-pointing triangles.
     */
    std::vector<Position> rsu;

    /**
     * Vector of left-side-down (LSD) positions used to apply the brush to. As the S2 grid is built up by triangles
     * pointing either upwards or downwards, this vector contains only positions of down-pointing triangles.
     */
    std::vector<Position> lsd;
};

#endif // BrushSettings_h__
