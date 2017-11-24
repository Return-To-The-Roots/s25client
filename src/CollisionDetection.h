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

#ifndef COLLISIONDETECTION_H_INCLUDED
#define COLLISIONDETECTION_H_INCLUDED

#include "Point.h"

struct Rect;

bool IsPointInRect(const Position& pt, const Rect& rect);
bool IsPointInRect(const int x, const int y, const Rect& rect);
bool IsPointInRect(const int x, const int y, const int rx, const int ry, const int rwidth, const int rheight);
bool DoRectsIntersect(const Rect& rect1, const Rect& rect2);

#endif // COLLISIONDETECTION_H_INCLUDED
