// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/Nation.h"
#include <cstdint>

/// Y-value for the point where the carrier disappears inside the building for each nation and building
constexpr helpers::MultiEnumArray<int8_t, Nation, BuildingType> DOOR_CONSTS = {
  {// Nubier
   {5, 10, 13, 0,  10, 0,  0,  0, 0,  6,  8, 8,  8,  8, 6,  0, 10, 10, 12, 14,
    9, 5,  11, 19, 19, 12, 18, 0, -6, 19, 0, 12, 11, 6, 10, 0, 0,  -1, 4,  13},
   // Japaner
   {9, 1,  5, 0,  12, 0,  0, 0, 0,  7,  8, 8,  8,  8, 5,  0, 10, 9,  5,  3,
    9, 10, 3, 12, 10, 13, 7, 0, -8, 14, 0, 11, 10, 9, 11, 0, 15, -7, -5, 16},
   // Römer
   {4, 6,  8,  0,  12, 0,  0,  0, 0,  -3, 8, 8, 8, 8, 6,  0, 10, 12,  14, 12,
    9, 12, 12, 16, 19, 14, 16, 0, -8, 17, 0, 6, 9, 8, 14, 6, 4,  -13, -8, 2},
   // Wikinger
   {10, 12, 11, 0,  11, 0,  0,  0, 0,  14, 9, 9, 9,  9,  4,  0, 10, 10, 10, 12,
    12, 19, 11, 13, 6,  11, 14, 0, -3, 11, 0, 9, 11, 10, 16, 0, 16, -6, -2, 10},
   // Babylonians
   {9, 5,  5, 0,  12, 0,  0, 0, 0,  7,  8, 8,  8,  8, 5,  0,  10, 9,  5,  3,
    9, 10, 3, 12, 10, 13, 7, 0, -8, 14, 0, 11, 10, 9, 11, 11, 15, -7, -5, 16}}};
