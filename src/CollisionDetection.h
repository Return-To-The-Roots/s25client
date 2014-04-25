// $Id: CollisionDetection.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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


struct Rect;

// Punkt in einem Rechteck
bool Coll(const int x, const int y, const Rect& rect);
// Punkt in einem Rechteck
bool Coll(const int x, const int y, const int rx, const int ry, const int rwidth, const int rheight);
// 1D (2 Linien)
bool Coll(const int left1, const int right1, const int left2, const int right2);
// 2D (2 Rechtecke)
bool CollEdges(const Rect& rect1, const Rect& rect2);
// 2D (2 Rechtecke)
bool Coll(const Rect& rect1, const Rect& rec2);

#endif // COLLISIONDETECTION_H_INCLUDED
