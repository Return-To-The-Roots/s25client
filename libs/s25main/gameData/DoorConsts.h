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

#ifndef DOOR_CONSTS_H_
#define DOOR_CONSTS_H_

#include "helpers/MultiArray.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/Nation.h"

// Konstanten für den Punkt, ab dem die Träger "verschwinden" bei einem Gebäude, jeweils als Y-Angabe

const helpers::MultiArray<signed char, NUM_NATIONS, NUM_BUILDING_TYPES> DOOR_CONSTS = {
  {// Nubier
   {5, 10, 13, 0,  10, 0,  0,  0, 0,  6,  8, 8,  8,  8, 6,  0, 10, 10, 12, 14,
    9, 5,  11, 19, 19, 12, 18, 0, -6, 19, 0, 12, 11, 6, 10, 0, 0,  -1, 4,  13},
   // Japaner
   {9, 1, 5, 0, 12, 0, 0, 0, 0, 7, 8, 8, 8, 8, 5, 0, 10, 9, 5, 3, 9, 10, 3, 12, 10, 13, 7, 0, -8, 14, 0, 11, 10, 9, 11, 0, 15, -7, -5, 16},
   // Römer
   {4, 6,  8,  0,  12, 0,  0,  0, 0,  -3, 8, 8, 8, 8, 6,  0, 10, 12,  14, 12,
    9, 12, 12, 16, 19, 14, 16, 0, -8, 17, 0, 6, 9, 8, 14, 6, 4,  -13, -8, 2},
   // Wikinger
   {10, 12, 11, 0,  11, 0,  0,  0, 0,  14, 9, 9, 9,  9,  4,  0, 10, 10, 10, 12,
    12, 19, 11, 13, 6,  11, 14, 0, -3, 11, 0, 9, 11, 10, 16, 0, 16, -6, -2, 10},
   // Babylonians
   {9, 5,  5, 0,  12, 0,  0, 0, 0,  7,  8, 8,  8,  8, 5,  0,  10, 9,  5,  3,
    9, 10, 3, 12, 10, 13, 7, 0, -8, 14, 0, 11, 10, 9, 11, 11, 15, -7, -5, 16}}};

#endif
