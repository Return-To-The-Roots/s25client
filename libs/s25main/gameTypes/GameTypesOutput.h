// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#pragma once

#include "gameTypes/Direction.h"
#include "gameTypes/FoWNode.h"
#include "gameTypes/MapTypes.h"
#include <ostream>

#define RTTR_CASE_OUT(Enum, Enumerator) \
    case Enum::Enumerator: os << #Enumerator; break

// LCOV_EXCL_START
inline std::ostream& operator<<(std::ostream& os, const Direction& dir)
{
    switch(dir.native_value())
    {
        RTTR_CASE_OUT(Direction, WEST);
        RTTR_CASE_OUT(Direction, NORTHWEST);
        RTTR_CASE_OUT(Direction, NORTHEAST);
        RTTR_CASE_OUT(Direction, EAST);
        RTTR_CASE_OUT(Direction, SOUTHEAST);
        RTTR_CASE_OUT(Direction, SOUTHWEST);
    }
    return os;
}
inline std::ostream& operator<<(std::ostream& os, const RoadDir& dir)
{
    switch(dir)
    {
        RTTR_CASE_OUT(RoadDir, East);
        RTTR_CASE_OUT(RoadDir, SouthEast);
        RTTR_CASE_OUT(RoadDir, SouthWest);
    }
    return os;
}
inline std::ostream& operator<<(std::ostream& os, const PointRoad& road)
{
    switch(road)
    {
        RTTR_CASE_OUT(PointRoad, None);
        RTTR_CASE_OUT(PointRoad, Normal);
        RTTR_CASE_OUT(PointRoad, Donkey);
        RTTR_CASE_OUT(PointRoad, Boat);
    }
    return os;
}
inline std::ostream& operator<<(std::ostream& os, const BorderStonePos& road)
{
    switch(road)
    {
        RTTR_CASE_OUT(BorderStonePos, OnPoint);
        RTTR_CASE_OUT(BorderStonePos, HalfEast);
        RTTR_CASE_OUT(BorderStonePos, HalfSouthEast);
        RTTR_CASE_OUT(BorderStonePos, HalfSouthWest);
    }
    return os;
}
// LCOV_EXCL_STOP

#undef RTTR_CASE_OUT
