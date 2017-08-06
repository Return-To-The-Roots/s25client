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

#ifndef Direction_Output_h__
#define Direction_Output_h__

#include "gameTypes/Direction.h"
#include <ostream>

inline std::ostream& operator<<(std::ostream& os, const Direction& dir)
{
    switch(Direction::Type(dir))
    {
        case Direction::WEST: os << "WEST"; break;
        case Direction::NORTHWEST: os << "NORTHWEST"; break;
        case Direction::NORTHEAST: os << "NORTHEAST"; break;
        case Direction::EAST: os << "EAST"; break;
        case Direction::SOUTHEAST: os << "SOUTHEAST"; break;
        case Direction::SOUTHWEST: os << "SOUTHWEST"; break;
    }
    return os;
}

#endif // Direction_Output_h__
