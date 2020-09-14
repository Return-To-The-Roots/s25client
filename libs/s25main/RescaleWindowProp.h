// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

/// Functor or static method to scale a window propertie from the real coordinates
/// (relative to 800x600 resolution) to new coordinates given a size
struct ScaleWindowPropUp
{
    Extent size;
    ScaleWindowPropUp(const Extent& size) : size(size) {}

    template<typename T_Pt>
    static T_Pt scale(const T_Pt& value, const Extent& size);
    template<typename T_Pt>
    T_Pt operator()(const T_Pt& value) const;
};

/// Rescales a window's properties (size or positions)
struct RescaleWindowProp
{
    Extent oldSize, newSize;
    RescaleWindowProp(Extent oldSize, Extent newSize) : oldSize(oldSize), newSize(newSize) {}
    /// Scale the point or size from beeing relative to the oldSize to relative to the newSize
    template<typename T_Pt>
    T_Pt operator()(const T_Pt& oldValue) const;
};

template<typename T_Pt>
inline T_Pt ScaleWindowPropUp::scale(const T_Pt& value, const Extent& sizeToScale)
{
    return T_Pt(value * sizeToScale / Extent(800, 600));
}

template<typename T_Pt>
inline T_Pt ScaleWindowPropUp::operator()(const T_Pt& value) const
{
    return scale(value, size);
}

template<typename T_Pt>
inline T_Pt RescaleWindowProp::operator()(const T_Pt& oldValue) const
{
    T_Pt realValue(oldValue.x * 800 / oldSize.x, oldValue.y * 600 / oldSize.y);
    // Check for rounding errors
    T_Pt checkValue = ScaleWindowPropUp::scale(realValue, oldSize);
    if(checkValue.x < oldValue.x)
        realValue.x++;
    if(checkValue.y < oldValue.y)
        realValue.y++;
    return ScaleWindowPropUp::scale(realValue, newSize);
}
