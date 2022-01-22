// Copyright (C) 2005 - 2022 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PointOutput.h"
#include "Rect.h"
#include <iostream>

template<typename T>
std::ostream& operator<<(std::ostream& out, const RectBase<T>& rect)
{
    return out << "[Origin: " << rect.getOrigin() << " Size:" << rect.getSize() << "]"; // *1 to convert chars to int
}
