// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

/// Buildingqualities
enum class BuildingQuality : uint8_t
{
    Nothing,
    Flag,
    Hut,
    House,
    Castle,
    Mine,
    Harbor
};
constexpr auto maxEnumValue(BuildingQuality)
{
    return BuildingQuality::Harbor;
}

/// Return true iff the BQ found matches a required BQ. E.g. A building with a given size can be constructed on a given
/// node
inline bool canUseBq(BuildingQuality bqIs, BuildingQuality bqRequired)
{
    // Exact match -> OK
    if(bqIs == bqRequired)
        return true;
    // Harbor spots allows everything but mines
    if(bqIs == BuildingQuality::Harbor)
        return bqRequired != BuildingQuality::Mine;
    // Not a special bq (mine/harbor) and we require less then we have -> OK
    return static_cast<uint8_t>(bqIs) < static_cast<uint8_t>(BuildingQuality::Mine)
           && static_cast<uint8_t>(bqRequired) < static_cast<uint8_t>(bqIs);
}
