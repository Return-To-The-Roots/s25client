// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

/// Building qualities
enum class BuildingQuality : uint8_t
{
    Nothing,
    Flag,
    Mine,
    Hut,
    House,
    Castle,
    Harbor,
};
constexpr auto maxEnumValue(BuildingQuality)
{
    return BuildingQuality::Harbor;
}

/// Return true iff the BQ found matches a required BQ. E.g. A building with a given size can be constructed on a given
/// node
inline bool canUseBq(BuildingQuality bqIs, BuildingQuality bqRequired)
{
    // Mines can only be build at mine spots
    if(bqRequired == BuildingQuality::Mine)
        return bqIs == BuildingQuality::Mine;
    // No mine required and we require at most what we have -> OK (mine is placed above flag!)
    return bqRequired <= bqIs;
}
