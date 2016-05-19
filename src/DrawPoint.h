// Copyright (c) 2005 - 2016 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef DrawPoint_h__
#define DrawPoint_h__

#include "Point.h"

/// Type used for specifying drawing offsets and coordinates. Signed base type!
typedef Point<int> DrawPoint;
/// Helper struct to allow use in array initializers till C++11.
/// Implicitly convertible to DrawPoint
struct DrawPointInit
{
    typedef typename DrawPoint::ElementType ElementType;
    const ElementType x;
    const ElementType y;
    operator DrawPoint() const { return DrawPoint(x, y); }
};
// Workaround for +/- if both arguments are DrawPointInit (DrawPoint::+ is not found via ADL)
inline DrawPoint operator+(const DrawPointInit& lhs, const DrawPointInit& rhs)
{
    return DrawPoint(lhs) + rhs;
}

inline DrawPoint operator-(const DrawPointInit& lhs, const DrawPointInit& rhs)
{
    return DrawPoint(lhs) - rhs;
}

#endif // DrawPoint_h__
