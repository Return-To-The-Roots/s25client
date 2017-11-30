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
#ifndef MAP_GEOMETRY_H_
#define MAP_GEOMETRY_H_

#include "Point.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"

Position GetNeighbour(const Position& pt, const Direction dir);
Position GetNeighbour2(Position pt, unsigned dir);
MapPoint MakeMapPoint(Position pt, const MapExtent& size);
/// Return the position of the node on a flat map
Position GetNodePos(MapPoint pt);
Position GetNodePos(Position pt);
/// Return the height adjusted point
Position GetNodePos(MapPoint pt, uint8_t height);

#endif //! MAP_GEOMETRY_H_
