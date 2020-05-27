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

#ifndef BrushDirection_h__
#define BrushDirection_h__

#include "randomMaps/algorithm/BrushSize.h"
#include "randomMaps/algorithm/RandomUtility.h"
#include "gameTypes/MapCoordinates.h"

class BrushDirection
{
public:
    
    static BrushSettings West()
    {
        return BrushSettings({-1,-1,-1,0,-1,1},{-1,0,-2,1,-1,2});
    }

    static BrushSettings East()
    {
        return BrushSettings({1,-1,2,0,1,1},{1,0,1,1,1,2});
    }

    static BrushSettings North()
    {
        return BrushSettings({-1,-1,0,-1,1,-1},{-1,-1,0,-1});
    }
    
    static BrushSettings South()
    {
        return BrushSettings({0,2,1,2},{-1,2,0,2,1,2});
    }
    
    static BrushSettings NorthEast()
    {
        return BrushSettings({1,-1,2,0},{0,-1,1,0,1,1});
    }
    
    static BrushSettings NorthWest()
    {
        return BrushSettings({-1,0,-1,-1},{-2,1,-1,0,-1,-1});
    }

    static BrushSettings SouthEast()
    {
        return BrushSettings({-1,0,-1,1,0,2},{-2,1,-1,2});
    }
    
    static BrushSettings SouthWest()
    {
        return BrushSettings({2,0,1,1,1,2},{1,1,1,2});
    }
};

#endif // BrushDirection_h__
