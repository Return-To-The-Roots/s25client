// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
