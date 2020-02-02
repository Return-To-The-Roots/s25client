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
#include <utility>

namespace rttr {
namespace mapGenerator {

using Indices = std::vector<int>;

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
    BrushSettings(std::vector<Position> rsuArg, std::vector<Position> lsdArg);
    
    /**
     * Vector of right-side-up (RSU) positions used to apply the brush to. As the S2 grid is built up by triangles
     * pointing either upwards or downwards, this vector contains only positions of up-pointing triangles.
     */
    const std::vector<Position> rsu;

    /**
     * Vector of left-side-down (LSD) positions used to apply the brush to. As the S2 grid is built up by triangles
     * pointing either upwards or downwards, this vector contains only positions of down-pointing triangles.
     */
    const std::vector<Position> lsd;
};

/**
 * Initializes the right-side-up ("rsu") and the left-side-down ("lsd") vectors from the specified integer vectors.
 * The integer values of the vectors are being interpret as x-y-pairs like this: [ x0, y0, x1, y1, x2, y2, ... ]
 * @param rsuArg integer vector used to populate the x- and y-components of the "rsu" vector
 * @param lsdArg integer vector used to populate the x- and y-components of the "lsd" vector
 */
BrushSettings CreateBrushSettings(const std::vector<int>& rsuArg, const std::vector<int>& lsdArg);

/**
 * RSU indices for a medium size brush.
 */
const Indices MediumRsu = {-1,-1,0,-1,1,-1,-1,0,0,0,1,0,2,0,-1,1,0,1,1,1,0,2,1,2};

/**
 * LSD indices for a medium size brush.
 */
const Indices MediumLsd = {-1,-1,0,-1,-1,0,0,0,1,0,-2,1,-1,1,0,1,1,1,-1,2,0,2,1,2};

/**
 * RSU indices for a large size brush.
 */
const Indices LargeRsu = {-1,-2,0,-2,1,-2,2,-2,-2,-1,-1,-1,0,-1,1,-1,2,-1,-2,0,-1,0,0,0,1,0,2,0,3,0,
-2,1,-1,1,0,1,1,1,2,1,-1,2,0,2,1,2,2,2,-1,3,0,3,1,3};

/**
 * LSD indices for a large size brush.
 */
const Indices LargeLsd = {-2,-1,-1,-1,0,-1,1,-1,-2,0,-1,0,0,0,1,0,2,0,-3,1,-2,1,-1,1,0,1,1,1,2,1,
-2,2,-1,2,0,2,1,2,2,2,-2,3,-1,3,0,3,1,3};

/**
 * RSU indices for a huge size brush.
 */
const Indices HugeRsu = {-2,-3,-1,-3,0,-3,1,-3,2,-3,-2,-2,-1,-2,0,-2,1,-2,2,-2,3,-2,-3,-1,-2,-1,-1,-1,
0,-1,1,-1,2,-1,3,-1,-3,0,-2,0,-1,0,0,0,1,0,2,0,3,0,4,0,-3,1,-2,1,-1,1,0,1,1,1,2,1,3,1,-2,2,-1,2,0,2,
1,2,2,2,3,2,-2,3,-1,3,0,3,1,3,2,3,-1,4,0,4,1,4,2,4};

/**
 * LSD indices for a huge size brush.
 */
const Indices HugeLsd = {-2,-3,-1,-3,0,-3,1,-3,-2,-2,-1,-2,0,-2,1,-2,2,-2,-3,-1,-2,-1,-1,-1,0,-1,1,-1,
2,-1,-3,0,-2,0,-1,0,0,0,1,0,2,0,3,0,-4,1,-3,1,-2,1,-1,1,0,1,1,1,2,1,3,1,-3,2,-2,2,-1,2,0,2,1,2,2,2,
3,2,-3,3,-2,3,-1,3,0,3,1,3,2,3,-2,4,-1,4,0,4,1,4,2,4};

const BrushSettings West      = CreateBrushSettings({-1,-1,-1, 0,-1, 1},{-1, 0,-2, 1,-1,2});
const BrushSettings East      = CreateBrushSettings({ 1,-1, 2, 0, 1, 1},{ 1, 0, 1, 1, 1,2});
const BrushSettings North     = CreateBrushSettings({-1,-1, 0,-1, 1,-1},{-1,-1, 0,-1});
const BrushSettings South     = CreateBrushSettings({ 0, 2, 1, 2},{-1, 2, 0, 2, 1, 2});
const BrushSettings NorthEast = CreateBrushSettings({ 1,-1, 2, 0},{ 0,-1, 1, 0, 1, 1});
const BrushSettings NorthWest = CreateBrushSettings({-1, 0,-1,-1},{-2, 1,-1, 0,-1,-1});
const BrushSettings SouthEast = CreateBrushSettings({-1, 0,-1, 1, 0, 2},{-2, 1,-1, 2});
const BrushSettings SouthWest = CreateBrushSettings({ 2, 0, 1, 1, 1, 2},{ 1, 1, 1, 2});

const BrushSettings Small = CreateBrushSettings({0,0,1,0,0,1},{0,0,-1,1,0,1});
const BrushSettings Medium = CreateBrushSettings(MediumRsu, MediumLsd);
const BrushSettings Large = CreateBrushSettings(LargeRsu, LargeLsd);
const BrushSettings Huge = CreateBrushSettings(HugeRsu, HugeLsd);

const auto BrushDirections = {
    East,
    West,
    North,
    NorthEast,
    NorthWest,
    South,
    SouthEast,
    SouthWest
};

}}

#endif // BrushSettings_h__
