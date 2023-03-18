// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

enum class MapType : uint8_t
{
    OldMap,
    Savegame
};
constexpr auto maxEnumValue(MapType)
{
    return MapType::Savegame;
}
