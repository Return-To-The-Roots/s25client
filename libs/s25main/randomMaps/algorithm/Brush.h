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

#ifndef Brush_h__
#define Brush_h__

#include "randomMaps/algorithm/BrushSettings.h"
#include "gameTypes/MapCoordinates.h"

/**
 * Utility class for modifying an S2 grid with a paint brush.
 * TParam defines the type of argument passed to the paint function of the brush while painting.
 */
template <typename TParam>
class Brush
{
    public:
    
    /**
     * Creates a new brush with the specified brush function.
     * @param brushFunc function applied while painting with the brush (params: param, index, RSU/LSD)
     */
    Brush(void (*brushFunc)(TParam&, int, bool)) : _brushFunc(brushFunc)
    {
        
    }

    /**
     * Applies the brush to the map at the specified position.
     * @param params additional parameters passed to the brush function
     * @param settings brush settings to use for painting
     * @param position center of the brush
     * @param size size of the grid to paint on
     */
    void Paint(TParam& params, BrushSettings settings, Position position, const MapExtent& size)
    {
        Apply(true, params, settings.rsu, position.x, position.y, size);
        Apply(false, params, settings.lsd, position.x, position.y, size);
    }
    
    private:
    
    /**
     * Applies the brush function with the specified parameters.
     * @param grid grid to apply the paint function to
     * @param params brush parameters
     * @param offset offset values for the positions of the brush
     * @param x x-coordinate of the painting center
     * @param y y-coordinate of the painting center
     */
    void Apply(bool rsu, TParam& params, std::vector<Position> offsets, int x, int y, const MapExtent& size)
    {
        for (auto i = 0u; i < offsets.size(); i++)
        {
            // shift for odd y-values
            int k = offsets[i].y < 0 ? -offsets[i].y : offsets[i].y;
            
            // point to apply the paint function to
            int ry = y + offsets[i].y;
            int rx = x + offsets[i].x + (k % 2 == 1 ? y % 2 : 0);
            
            // array index for the current painting position
            int index = (rx & (size.x - 1)) + (ry & (size.y - 1)) * size.x;
            
            // apply the brush function to the indexed grid point
            _brushFunc(params, index, rsu);
        }
    }
    
    /**
     * Function applied while painting with the brush (params: generic parameters, index, RSU/LSD).
     * 1nd parameter: additional parameters used for painting
     * 2rd parameter: grid index to modify
     * 3th parameter: right-side-up (true) or left-side-down (false)
     */
    void (*_brushFunc)(TParam&, int, bool);
};

#endif // Brush_h__
