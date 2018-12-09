// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef Rect_h__
#define Rect_h__

#include "Point.h"
#include <boost/type_traits/common_type.hpp>
#include <boost/type_traits/conditional.hpp>
#include <boost/type_traits/make_unsigned.hpp>

/// Describe a rectangular shape with dimensions:
/// x: [left, right), y: [top, bottom)
template<typename T>
struct RectBase
{
    typedef Point<T> position_type;
    typedef typename boost::conditional<boost::is_integral<T>::value, boost::make_unsigned<T>, boost::common_type<T> >::type::type
      extent_elem_type;
    typedef Point<extent_elem_type> extent_type;
    T left, top, right, bottom;
    RectBase() : left(0), top(0), right(0), bottom(0) {}
    RectBase(T left, T top, extent_elem_type width, extent_elem_type height);
    RectBase(const position_type& origin, extent_elem_type width, extent_elem_type height);
    RectBase(const position_type& origin, const extent_type& size);
    position_type getOrigin() const { return position_type(left, top); }
    position_type getEndPt() const { return position_type(right, bottom); }
    void setOrigin(const position_type&);
    extent_type getSize() const;
    void setSize(const extent_type& newSize);
    void move(const position_type& offset);
    static RectBase move(RectBase rect, const position_type& offset);
};

typedef RectBase<int> Rect;

template<typename T>
RectBase<T>::RectBase(T left, T top, extent_elem_type width, extent_elem_type height) : left(left), top(top)
{
    setSize(extent_type(width, height));
}
template<typename T>
RectBase<T>::RectBase(const position_type& lt, extent_elem_type width, extent_elem_type height) : left(lt.x), top(lt.y)
{
    setSize(extent_type(width, height));
}
template<typename T>
RectBase<T>::RectBase(const position_type& lt, const extent_type& size) : left(lt.x), top(lt.y)
{
    setSize(extent_type(size));
}
template<typename T>
void RectBase<T>::setOrigin(const position_type& pos)
{
    move(pos - getOrigin());
}
template<typename T>
typename RectBase<T>::extent_type RectBase<T>::getSize() const
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

#endif // Rect_h__
