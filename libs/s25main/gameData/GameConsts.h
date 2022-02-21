// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include "helpers/make_array.h"
#include "gameTypes/GameSettingTypes.h"
#include <chrono>
#include <limits>

using namespace std::chrono_literals;
/// Geschwindigkeitsabstufungen - Längen der GFs in ms
constexpr helpers::EnumArray<std::chrono::duration<unsigned, std::milli>, GameSpeed> SUPPRESS_UNUSED
  SPEED_GF_LENGTHS = {{80ms, 60ms, 50ms, 40ms, 30ms}};

/// Normal speed as reference speed for ingame time computations
constexpr GameSpeed referenceSpeed = GameSpeed::Normal;

/// Reichweite der Bergarbeiter
constexpr unsigned MINER_RADIUS = 2;
constexpr unsigned MINER_RADIUS_SETTLERSIV = 1u;

/// maximum quantity for ores
constexpr unsigned MINER_MAX_QUANTITY = 7u;

/// Konstante für die Pfadrichtung bei einer Schiffsverbindung
constexpr unsigned char SHIP_DIR = 100;
constexpr unsigned char INVALID_DIR = 0xFF;
constexpr unsigned SUPPRESS_UNUSED NO_MAX_LEN = std::numeric_limits<unsigned>::max();

/// tournament modes
constexpr auto SUPPRESS_UNUSED TOURNAMENT_MODES_DURATION = helpers::make_array(30, 60, 90, 120, 240);
static_assert(TOURNAMENT_MODES_DURATION.size() == NUM_TOURNAMENT_MODES, "!");
