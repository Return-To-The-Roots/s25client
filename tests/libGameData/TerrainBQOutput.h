// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameData/TerrainDesc.h"
#include <array>
#include <iostream>

inline std::ostream& operator<<(std::ostream& stream, TerrainBQ bq)
{
    static const std::array<const char*, 6> bqNames = {{"Nothing", "Danger", "Flag", "Castle", "Mine"}};
    return stream << bqNames[static_cast<unsigned>(bq)];
}
