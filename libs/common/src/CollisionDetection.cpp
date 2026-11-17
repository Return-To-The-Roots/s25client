// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CollisionDetection.h"

bool isPointInRect(const Position& pt, const Rect& rect)
{
    return (pt.x >= rect.left && pt.x < rect.right && pt.y >= rect.top && pt.y < rect.bottom);
}

bool isPointInRect(const int x, const int y, const Rect& rect)
{
    return isPointInRect(Position(x, y), rect);
}

bool isPointInRect(const int x, const int y, const int rx, const int ry, const int rwidth, const int rheight)
{
    return isPointInRect(Position(x, y), Rect(Position(rx, ry), Extent(rwidth, rheight)));
}

bool doRectsIntersect(const Rect& rect1, const Rect& rect2)
{
    // Size = 0 -> No intersection possible
    if(rect1.getSize() == Extent(0, 0) || rect2.getSize() == Extent(0, 0))
        return false;
    return (isPointInRect(rect1.left, rect1.top, rect2) || isPointInRect(rect1.right - 1, rect1.top, rect2)
            || isPointInRect(rect1.left, rect1.bottom - 1, rect2)
            || isPointInRect(rect1.right - 1, rect1.bottom - 1, rect2));
}
