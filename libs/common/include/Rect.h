// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Point.h"
#include "RTTR_Assert.h"
#include <type_traits>

#ifdef RTTR_ENABLE_ASSERTS
#    define CONSTEXPR_IF_NOASSERT
#else
#    define CONSTEXPR_IF_NOASSERT constexpr
#endif

/// Describe a rectangular shape with dimensions:
/// x: [left, right), y: [top, bottom)
template<typename T>
struct RectBase
{
    using position_type = Point<T>;
    using extent_elem_type =
      typename std::conditional_t<std::is_integral_v<T>, std::make_unsigned<T>, std::common_type<T>>::type;
    using extent_type = Point<extent_elem_type>;
    T left, top, right, bottom;
    constexpr RectBase() : RectBase(position_type::all(0), extent_type::all(0)) {}
    constexpr RectBase(T left, T top, extent_elem_type width, extent_elem_type height)
        : RectBase(position_type(left, top), extent_type(width, height))
    {}
    constexpr RectBase(const position_type& origin, extent_elem_type width, extent_elem_type height)
        : RectBase(origin, extent_type(width, height))
    {}
    constexpr RectBase(const position_type& origin, const extent_type& size);
    constexpr position_type getOrigin() const { return position_type(left, top); }
    constexpr position_type getEndPt() const { return position_type(right, bottom); }
    void setOrigin(const position_type&);
    CONSTEXPR_IF_NOASSERT extent_type getSize() const;
    void setSize(const extent_type& newSize);
    void move(const position_type& offset);
    static RectBase move(RectBase rect, const position_type& offset);

    constexpr bool operator==(const RectBase& rhs) const
    {
        return left == rhs.left && top == rhs.top && right == rhs.right && bottom == rhs.bottom;
    }
    constexpr bool operator!=(const RectBase& rhs) const { return !(*this == rhs); }
};

using Rect = RectBase<int>;

template<typename T>
constexpr RectBase<T>::RectBase(const position_type& origin, const extent_type& size)
    : left(origin.x), top(origin.y), right(origin.x + size.x), bottom(origin.y + size.y)
{}
template<typename T>
void RectBase<T>::setOrigin(const position_type& pos)
{
    move(pos - getOrigin());
}
template<typename T>
CONSTEXPR_IF_NOASSERT typename RectBase<T>::extent_type RectBase<T>::getSize() const
{
    RTTR_Assert(left <= right);
    RTTR_Assert(top <= bottom);
    return extent_type(right - left, bottom - top);
}
template<typename T>
void RectBase<T>::setSize(const extent_type& newSize)
{
    right = left + newSize.x;
    bottom = top + newSize.y;
}
template<typename T>
void RectBase<T>::move(const position_type& offset)
{
    left += offset.x;
    right += offset.x;
    top += offset.y;
    bottom += offset.y;
}
template<typename T>
RectBase<T> RectBase<T>::move(RectBase<T> rect, const typename RectBase<T>::position_type& offset)
{
    rect.move(offset);
    return rect;
}
