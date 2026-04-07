// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/// Functor or static method to (Re-) scale window properties from the real coordinates
/// (relative to reference resolution) to new coordinates given a size
struct ScaleWindowProp
{
    /// ScalingPercentage Type for proportional scaling, capped range from 0 - 100%
    /// 100%: scales GUI element base size fully proportional to target size
    /// 50%: considers only half of target size
    /// 0% does not apply any scaling at all
    /// The target size is normalized against the reference solution
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
    T_Pt diff(sizeToScale - REFERENCE_RESOLUTION);
    T_Pt limScaledValue(value * (sizeToScale - diff + (diff * limfactors / 100)) / REFERENCE_RESOLUTION);
    return limScaledValue;
}

template<typename T_Pt>
inline T_Pt ScaleWindowProp::operator()(const T_Pt& value) const
{
    return scale(value, size, LimitFactors(100, 100));
}

template<typename T_Pt>
inline T_Pt ScaleWindowProp::operator()(const T_Pt& oldValue, const LimitFactors& limfactors) const
{
    T_Pt realValue;
    T_Pt diff(oldSize - REFERENCE_RESOLUTION);
    T_Pt limUnscaleValue(oldValue.x * REFERENCE_RESOLUTION.x / (oldSize.x - diff.x + (diff.x * limfactors.x / 100)),
                            oldValue.y * REFERENCE_RESOLUTION.y / (oldSize.y - diff.y + (diff.y * limfactors.y / 100)));
    realValue = limUnscaleValue;
    // Check for rounding errors
    T_Pt checkValue = ScaleWindowProp::scale(realValue, oldSize, limfactors);
    if(checkValue.x < oldValue.x)
        realValue.x++;
    if(checkValue.y < oldValue.y)
        realValue.y++;
    return ScaleWindowProp::scale(realValue, newSize, limfactors);
}
