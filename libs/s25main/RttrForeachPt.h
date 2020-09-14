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

#pragma once

/// Iterate over all points of an area using a point of TYPE named "pt"
/// WIDTH and HEIGHT is evaluated at most once
/// Use like:
///     RTTR_FOREACH_PT(Position, world.GetSize()) {
///         std::cout << pt.x << "/" << pt.y;
///     }
#define RTTR_FOREACH_PT(TYPE, SIZE)                                             \
    /* Create scoped temporaries holding width and height by */                 \
    /* using assignment in if to save potential accesses */                     \
    if(auto rttrForeachPtWidth = static_cast<TYPE::ElementType>((SIZE).x))      \
        if(auto rttrForeachPtHeight = static_cast<TYPE::ElementType>((SIZE).y)) \
            for(TYPE pt(0, 0); pt.y < rttrForeachPtHeight; ++pt.y)              \
                for(pt.x = 0; pt.x < rttrForeachPtWidth; ++pt.x)
