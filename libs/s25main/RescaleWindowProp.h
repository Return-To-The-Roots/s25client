// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/// Functor or static method to (Re-) scale window properties from the real coordinates
/// (relative to reference resolution) to new coordinates given a size
struct ScaleWindowProp
{
    /// Type for describing limiting scaling factors for GUI elements (unsigned type)
    /// Limits scaling from Base Resolution 800x600 to fit screen size in a granularity of 10
    /// Valid range is from 1 up to 10, values outside the range don't impose any limiting
    /// e. g. Value 1: scales 1/10 of the difference between screensize and Reference Resolution
    using LimitFactors = Point<unsigned>;

    /// Reference Resolution used
    static constexpr Extent REFERENCE_RESOLUTION = Extent(800, 600);

    Extent size, oldSize, newSize;
    ScaleWindowProp(const Extent& size) : size(size) {}
    ScaleWindowProp(Extent oldSize, Extent newSize) : oldSize(oldSize), newSize(newSize) {}

    template<typename T_Pt>
    static T_Pt scale(const T_Pt& value, const Extent& size, const LimitFactors& limfactors);

    template<typename T_Pt>
    T_Pt operator()(const T_Pt& value) const;
    /// Scale the point or size from beeing relative to the oldSize to relative to the newSize
    template<typename T_Pt>
    T_Pt operator()(const T_Pt& oldValue, const LimitFactors& limfactors) const;
};

template<typename T_Pt>
inline T_Pt ScaleWindowProp::scale(const T_Pt& value, const Extent& sizeToScale, const LimitFactors& limfactors)
{
    if(limfactors.x > 0 && limfactors.x < 11 && limfactors.y > 0 && limfactors.y < 11)
    {
        T_Pt diff(sizeToScale - REFERENCE_RESOLUTION);
        T_Pt limScaledValue(value * (sizeToScale - diff * limfactors / 10) / REFERENCE_RESOLUTION);
        return limScaledValue;
    } else
    {
        T_Pt scaledValue(value * sizeToScale / REFERENCE_RESOLUTION);
        return scaledValue;
    }
}

template<typename T_Pt>
inline T_Pt ScaleWindowProp::operator()(const T_Pt& value) const
{
    return scale(value, size, LimitFactors(0, 0));
}

template<typename T_Pt>
inline T_Pt ScaleWindowProp::operator()(const T_Pt& oldValue, const LimitFactors& limfactors) const
{
    T_Pt realValue;
    if(limfactors.x > 0 && limfactors.x < 11 && limfactors.y > 0 && limfactors.y < 11)
    {
        T_Pt diff(oldSize - REFERENCE_RESOLUTION);
        T_Pt limUnscaleValue(oldValue.x * REFERENCE_RESOLUTION.x / (oldSize.x - (diff.x * limfactors.x / 10)),
                             oldValue.y * REFERENCE_RESOLUTION.y / (oldSize.y - (diff.y * limfactors.y / 10)));
        realValue = limUnscaleValue;
    } else
    {
        T_Pt unscaleValue(oldValue.x * REFERENCE_RESOLUTION.x / oldSize.x, oldValue.y * REFERENCE_RESOLUTION.y / oldSize.y);
        realValue = unscaleValue;
    }
    // Check for rounding errors
    T_Pt checkValue = ScaleWindowProp::scale(realValue, oldSize, limfactors);
    if(checkValue.x < oldValue.x)
        realValue.x++;
    if(checkValue.y < oldValue.y)
        realValue.y++;
    return ScaleWindowProp::scale(realValue, newSize, limfactors);
}
