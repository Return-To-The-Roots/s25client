// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

struct Rect
{
    int left, top, right, bottom;
    Rect(): left(0), top(0), right(0), bottom(0){}
    Rect(int left, int top, int width, int height): left(left), top(top), right(left + width), bottom(top + height){}
    Rect(const Point<int>& lt, int width, int height): left(lt.x), top(lt.y), right(left + width), bottom(top + height){}
    Rect(const Point<int>& lt, const Point<int>& size): left(lt.x), top(lt.y), right(left + size.x), bottom(top + size.y){}
};

#endif // Rect_h__
