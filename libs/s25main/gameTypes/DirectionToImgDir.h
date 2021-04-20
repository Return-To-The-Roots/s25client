// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
