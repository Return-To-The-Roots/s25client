// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "RTTR_Assert.h"
#include "gameTypes/Direction.h"
#include "libsiedler2/ImgDir.h"

inline libsiedler2::ImgDir toImgDir(Direction dir)
{
    using libsiedler2::ImgDir;
    switch(dir)
    {
        case Direction::West: return ImgDir::W;
        case Direction::NorthWest: return ImgDir::NW;
        case Direction::NorthEast: return ImgDir::NE;
        case Direction::East: return ImgDir::E;
        case Direction::SouthEast: return ImgDir::SE;
        default: RTTR_Assert(dir == Direction::SouthWest); return ImgDir::SW;
    }
}
