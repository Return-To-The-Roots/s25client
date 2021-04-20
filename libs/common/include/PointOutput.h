// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Point.h"
#include <iostream>

template<typename T>
std::ostream& operator<<(std::ostream& out, const Point<T>& point)
{
    return out << "(" << point.x * 1 << ", " << point.y * 1 << ")"; // *1 to convert chars to int
}
