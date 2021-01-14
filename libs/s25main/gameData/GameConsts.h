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

#include "helpers/EnumArray.h"
#include "helpers/make_array.h"
#include "gameTypes/GameSettingTypes.h"
#include <limits>

using namespace std::chrono_literals;
/// Geschwindigkeitsabstufungen - Längen der GFs in ms
constexpr helpers::EnumArray<std::chrono::duration<unsigned, std::milli>, GameSpeed> SUPPRESS_UNUSED
  SPEED_GF_LENGTHS = {{80ms, 60ms, 50ms, 40ms, 30ms}};

/// Normal speed as reference speed for ingame time computations
constexpr GameSpeed referenceSpeed = GameSpeed::Normal;

/// Reichweite der Bergarbeiter
constexpr unsigned MINER_RADIUS = 2;

/// Konstante für die Pfadrichtung bei einer Schiffsverbindung
constexpr unsigned char SHIP_DIR = 100;
constexpr unsigned char INVALID_DIR = 0xFF;
constexpr unsigned SUPPRESS_UNUSED NO_MAX_LEN = std::numeric_limits<unsigned>::max();

/// tournament modes
constexpr auto SUPPRESS_UNUSED TOURNAMENT_MODES_DURATION = helpers::make_array(30, 60, 90, 120, 240);
static_assert(TOURNAMENT_MODES_DURATION.size() == NUM_TOURNAMENT_MODES, "!");
