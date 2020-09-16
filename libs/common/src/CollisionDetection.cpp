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

#include "CollisionDetection.h"

bool IsPointInRect(const Position& pt, const Rect& rect)
{
    return (pt.x >= rect.left && pt.x < rect.right && pt.y >= rect.top && pt.y < rect.bottom);
}

bool IsPointInRect(const int x, const int y, const Rect& rect)
{
    return IsPointInRect(Position(x, y), rect);
}

bool IsPointInRect(const int x, const int y, const int rx, const int ry, const int rwidth, const int rheight)
{
    return IsPointInRect(Position(x, y), Rect(Position(rx, ry), Extent(rwidth, rheight)));
}

bool DoRectsIntersect(const Rect& rect1, const Rect& rect2)
{
    // Size = 0 -> No intersection possible
    if(rect1.getSize() == Extent(0, 0) || rect2.getSize() == Extent(0, 0))
        return false;
    return (IsPointInRect(rect1.left, rect1.top, rect2) || IsPointInRect(rect1.right - 1, rect1.top, rect2)
            || IsPointInRect(rect1.left, rect1.bottom - 1, rect2)
            || IsPointInRect(rect1.right - 1, rect1.bottom - 1, rect2));
}
