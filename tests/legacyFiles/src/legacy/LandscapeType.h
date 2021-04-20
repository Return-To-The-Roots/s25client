// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

enum class Landscape : uint8_t
{
    GREENLAND,
    WASTELAND,
    WINTERWORLD
};

// Keep this in sync with LandscapeType
static const uint8_t NUM_LTS = 3;
