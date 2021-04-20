// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/GoodTypes.h"
#include <array>

/// List of all tools (correspond to buttons at IO:140-163)
const std::array<GoodType, NUM_TOOLS> SUPPRESS_UNUSED TOOLS = {
  GoodType::Tongs,      // Zange
  GoodType::Hammer,     // Hammer
  GoodType::Axe,        // Axt,
  GoodType::Saw,        // Säge
  GoodType::PickAxe,    // Spitzhacke
  GoodType::Shovel,     // Schaufel
  GoodType::Crucible,   // Schmelztiegel
  GoodType::RodAndLine, // Angel
  GoodType::Scythe,     // Sense
  GoodType::Cleaver,    // Beil
  GoodType::Rollingpin, // Nudelholz
  GoodType::Bow         // Bogen
};
