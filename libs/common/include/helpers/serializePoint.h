// Copyright (c) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

#include "Point.h"
#include "s25util/Serializer.h"
#include <type_traits>

namespace helpers {

template<typename T>
void pushPoint(Serializer& ser, Point<T> pt)
{
    ser.Push(pt.x);
    ser.Push(pt.y);
}

template<typename PointT>
auto popPoint(Serializer& ser)
{
    using ElementType = typename PointT::ElementType;
    std::remove_const_t<PointT> pt;
    pt.x = ser.Pop<ElementType>();
    pt.y = ser.Pop<ElementType>();
    return pt;
}

} // namespace helpers
