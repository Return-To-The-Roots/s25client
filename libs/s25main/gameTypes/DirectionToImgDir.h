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

#ifndef toImgDir_h__
#define toImgDir_h__

#include "RTTR_Assert.h"
#include "gameTypes/Direction.h"
#include "libsiedler2/ImgDir.h"

inline libsiedler2::ImgDir toImgDir(Direction dir)
{
    using libsiedler2::ImgDir;
    switch(dir.native_value())
    {
        case Direction::WEST: return ImgDir::W;
        case Direction::NORTHWEST: return ImgDir::NW;
        case Direction::NORTHEAST: return ImgDir::NE;
        case Direction::EAST: return ImgDir::E;
        case Direction::SOUTHEAST: return ImgDir::SE;
        default: RTTR_Assert(dir == Direction::SOUTHWEST); return ImgDir::SW;
    }
}

#endif // toImgDir_h__
