// Copyright (C) 2005 - 2023 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Point.h"
#include "helpers/mathFuncs.h"
#include <type_traits>

/// Represents the scale factors – precomputed from a percentage – to translate between screen and view dimensions
class GuiScale
{
public:
    constexpr GuiScale(unsigned percent) noexcept
        : percent_(percent), scaleViewToScreen_(percent_ / 100.f), scaleScreenToView_(1.f / scaleViewToScreen_)
    {}

    constexpr unsigned percent() const noexcept { return percent_; }
    constexpr float scaleFactor() const noexcept { return scaleViewToScreen_; }
    constexpr float invScaleFactor() const noexcept { return scaleScreenToView_; }

    /// Translate a scalar from screen to view space
    template<typename R = float>
    constexpr R screenToView(float val) const noexcept
    {
        if constexpr(std::is_integral_v<R>)
            return helpers::iround<R>(val * scaleScreenToView_);
        else
            return static_cast<R>(val * scaleScreenToView_);
    }

    /// Translate a point from screen to view space
    template<typename R = Position, typename T>
    constexpr R screenToView(Point<T> pt) const noexcept
    {
        return R(PointF(pt) * scaleScreenToView_);
    }

    /// Translate a scalar from view to screen space
    template<typename R = float>
    constexpr R viewToScreen(float val) const noexcept
    {
        if constexpr(std::is_integral_v<R>)
            return helpers::iround<R>(val * scaleViewToScreen_);
        else
            return static_cast<R>(val * scaleViewToScreen_);
    }

    /// Translate a point from view to screen space
    template<typename R = Position, typename T>
    constexpr R viewToScreen(Point<T> pt) const noexcept
    {
        return R(PointF(pt) * scaleViewToScreen_);
    }

private:
    unsigned percent_;        ///< GUI scaling factor in percent
    float scaleViewToScreen_; ///< Decimal GUI scaling factor
    float scaleScreenToView_; ///< Inverse of the decimal GUI scaling factor
};
