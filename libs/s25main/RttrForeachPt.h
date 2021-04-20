// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
