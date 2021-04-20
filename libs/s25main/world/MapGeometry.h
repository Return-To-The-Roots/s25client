// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "Point.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"

Position GetNeighbour(const Position& p, Direction dir);
Position GetNeighbour2(Position pt, unsigned dir);
MapPoint MakeMapPoint(Position pt, const MapExtent& size);
/// Return the position of the node on a flat map
Position GetNodePos(MapPoint pt);
Position GetNodePos(Position pt);
/// Return the height adjusted point
Position GetNodePos(MapPoint pt, uint8_t height);
