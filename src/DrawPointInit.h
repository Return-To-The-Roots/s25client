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

#pragma once

#ifndef DrawPointInit_h__
#define DrawPointInit_h__

#include "DrawPoint.h"

/// Helper struct to allow use in array initializers till C++11.
/// Implicitly convertible to DrawPoint
struct DrawPointInit
{
    typedef DrawPoint::ElementType ElementType;
    const ElementType x;
    const ElementType y;
    operator DrawPoint() const { return DrawPoint(x, y); }
};

// Helper for defining the operations
#define DEF_DRAWPOINT_OP(OP)                                                          \
    inline DrawPoint operator OP (const DrawPoint& lhs, const DrawPointInit& rhs)     \
    { return lhs OP DrawPoint(rhs);}                                                  \
    inline DrawPoint operator OP (const DrawPointInit& lhs, const DrawPoint& rhs)     \
    { return DrawPoint(lhs) OP rhs;}                                                  \
    inline DrawPoint operator OP (const DrawPointInit& lhs, const DrawPointInit& rhs) \
    { return DrawPoint(lhs) OP rhs;}

DEF_DRAWPOINT_OP(+)
DEF_DRAWPOINT_OP(-)
DEF_DRAWPOINT_OP(*)
DEF_DRAWPOINT_OP(/)

#undef DEF_DRAWPOINT_OP

#endif // DrawPointInit_h__
