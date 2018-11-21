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

/// Describe a rectangular shape with dimensions:
/// x: [left, right), y: [top, bottom)
struct Rect
{
    int left, top, right, bottom;
    Rect() : left(0), top(0), right(0), bottom(0) {}
    Rect(int left, int top, unsigned width, unsigned height);
    Rect(const Position& lt, unsigned width, unsigned height);
    Rect(const Position& lt, const Extent& size);
    Position getOrigin() const { return Position(left, top); }
    Position getEndPt() const { return Position(right, bottom); }
    void setOrigin(const Position& pos);
    Extent getSize() const;
    void setSize(const Extent& newSize);
    void move(const Position& offset);
    static Rect move(Rect rect, const Position& offset);
};

inline Rect::Rect(int left, int top, unsigned width, unsigned height) : left(left), top(top)
{
    setSize(Extent(width, height));
}
inline Rect::Rect(const Position& lt, unsigned width, unsigned height) : left(lt.x), top(lt.y)
{
    setSize(Extent(width, height));
}
inline Rect::Rect(const Position& lt, const Extent& size) : left(lt.x), top(lt.y)
{
    setSize(Extent(size));
}

inline void Rect::setOrigin(const Position& pos)
{
    move(pos - getOrigin());
}

inline Extent Rect::getSize() const
{
    RTTR_Assert(left <= right);
    RTTR_Assert(top <= bottom);
    return Extent(right - left, bottom - top);
}

inline void Rect::setSize(const Extent& newSize)
{
    right = left + newSize.x;
    bottom = top + newSize.y;
}

inline void Rect::move(const Position& offset)
{
    left += offset.x;
    right += offset.x;
    top += offset.y;
    bottom += offset.y;
}

inline Rect Rect::move(Rect rect, const Position& offset)
{
    rect.move(offset);
    return rect;
}

#endif // Rect_h__
