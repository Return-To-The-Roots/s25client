// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Point.h"
#include "s25util/Serializer.h"
#include <type_traits>

namespace helpers {

template<typename T>
void pushPoint(Serializer& ser, Point<T> pt)
{
    ser.push(pt.x);
    ser.push(pt.y);
}

template<typename PointT>
auto popPoint(Serializer& ser)
{
    using ElementType = typename PointT::ElementType;
    std::remove_const_t<PointT> pt;
    pt.x = ser.pop<ElementType>();
    pt.y = ser.pop<ElementType>();
    return pt;
}

} // namespace helpers
