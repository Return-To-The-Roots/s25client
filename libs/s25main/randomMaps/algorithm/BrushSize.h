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

#ifndef BrushSize_h__
#define BrushSize_h__

#include "randomMaps/algorithm/BrushSettings.h"
#include <set>

class BrushSize
{
public:
    
    static bool IsTiny(const BrushSettings& settings)
    {
        return
            (settings.lsd.size() == 1u && settings.rsu.empty()) ||
            (settings.rsu.size() == 1u && settings.lsd.empty());
    }
    
    static bool IsRsu(const BrushSettings& settings)
    {
        return !settings.rsu.empty();
    }
    
    static BrushSettings Swap (const BrushSettings& settings)
    {
        if (IsTiny(settings))
        {
            return Tiny(!IsRsu(settings));
        }
        
        return settings;
    }
    
    static BrushSettings Tiny(bool rsu)
    {
        if (rsu)
        {
            return BrushSettings({ 0,0 }, {});
        }
        else
        {
            return BrushSettings({}, { 0,0 });
        }
    }
    
    static BrushSettings Small()
    {
        return BrushSettings(
        {
            0,0,1,0,
            0,1
        },
        {
             0,0,
            -1,1,0,1
        });
    }

    static BrushSettings Medium()
    {
        return BrushSettings(
        {
            -1,-1,0,-1, 1,-1,
            -1, 0,0, 0, 1, 0, 2,0,
            -1, 1,0, 1, 1, 1,
             0, 2,1, 2
        },
        {
            -1,-1, 0,-1,
            -1, 0, 0, 0,1,0,
            -2, 1,-1, 1,0,1,1,1,
            -1, 2, 0, 2,1,2
        });
    }
    
    static BrushSettings Large()
    {
        return BrushSettings(
        {
            -1,-2,0,-2,1,-2,2,-2,
            -2,-1,-1,-1,0,-1,1,-1,2,-1,
            -2, 0,-1, 0,0, 0,1, 0,2, 0,3,0,
            -2, 1,-1, 1,0, 1,1, 1,2, 1,
            -1, 2,0, 2,1, 2,2, 2,
            -1, 3,0, 3,1, 3
        },
        {
            -2,-1,-1,-1,0,-1,1,-1,
            -2, 0,-1, 0,0, 0,1, 0,2,0,
            -3, 1,-2, 1,-1, 1,0, 1,1, 1,2,1,
            -2, 2,-1, 2,0, 2,1, 2,2,2,
            -2, 3,-1, 3,0, 3,1, 3
        });
    }
    
    static BrushSettings Huge()
    {
        return BrushSettings(
        {
            -2,-3,-1,-3,0,-3,1,-3,2,-3,
            -2,-2,-1,-2,0,-2,1,-2,2,-2,3,-2,
            -3,-1,-2,-1,-1,-1,0,-1,1,-1,2,-1,3,-1,
            -3, 0,-2, 0,-1, 0,0, 0,1, 0,2, 0,3, 0,4,0,
            -3, 1,-2, 1,-1, 1,0, 1,1, 1,2, 1,3, 1,
            -2, 2,-1, 2,0, 2,1, 2,2, 2,3, 2,
            -2, 3,-1, 3,0, 3,1, 3,2, 3,
            -1, 4,0, 4,1, 4,2, 4
        },
        {
            -2,-3,-1,-3,0,-3,1,-3,
            -2,-2,-1,-2,0,-2,1,-2,2,-2,
            -3,-1,-2,-1,-1,-1,0,-1,1,-1,2,-1,
            -3, 0,-2, 0,-1, 0,0, 0,1, 0,2, 0,3, 0,
            -4, 1,-3, 1,-2, 1,-1, 1,0, 1,1, 1,2, 1,3, 1,
            -3, 2,-2, 2,-1, 2,0, 2,1, 2,2, 2,3, 2,
            -3, 3,-2, 3,-1, 3,0, 3,1, 3,2, 3,
            -2, 4,-1, 4,0, 4,1, 4,2, 4
        });
    }
};

#endif // BrushSize_h__
